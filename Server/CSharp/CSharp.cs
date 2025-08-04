using System.Net;
using System.Net.Sockets;
using System.Text;

using DSharpPlus;
using DSharpPlus.Entities;
using DSharpPlus.EventArgs;
using DSharpPlus.Commands;
using DSharpPlus.Commands.Processors.TextCommands;
using DSharpPlus.Commands.Processors.TextCommands.Parsing;
using System.Reflection;
using System.Threading.Channels;

public class Program
{
    // Start of configurable stuff
    private static readonly string Prefix = "sc";
    private static ulong GUILD_ID = 1145236064596918304;
    private static ulong FORUM_ID = 1401679019531042826;
    // -TODO Move to json / file read?
    // End of configurable stuff

#pragma warning disable CS8618
    private static DiscordClient Bot;
    private static CommandsExtension Commands;
#pragma warning restore CS8618

    private static List<TcpClient> Clients = new List<TcpClient>();
    private static object LockObj = new object();

    private static List<string> DiscordMessages = new();
    private static List<string> GameMessages = new();

    public static async Task Main( string[] args )
    {
        // Get token if given from command line. otherwise ask the user by input.
        int TokenIndex = Array.IndexOf( args, "-token" );
        string? token = ( TokenIndex > -1 && TokenIndex < args.Length - 1 ) ? args[ TokenIndex + 1 ] : null;

        while( string.IsNullOrWhiteSpace( token ) )
        {
            Console.Write( "Input a valid bot TOKEN\n" );
            token = Console.ReadLine();
        }

        DiscordClientBuilder builder = DiscordClientBuilder.CreateDefault( token, 
            DiscordIntents.GuildMessages | DiscordIntents.MessageContents
        );

        builder.ConfigureEventHandlers( b => b
            .HandleMessageCreated( OnMessage )
        );

        builder.UseCommands(
            ( svc, cmds ) =>
            {
                Commands = cmds;

                TextCommandProcessor textProc = new TextCommandProcessor(
                    new TextCommandConfiguration
                    {
                        PrefixResolver = new DefaultPrefixResolver( true, Prefix ).ResolvePrefixAsync
                    }
                );

                cmds.AddProcessor( textProc );
            },
            new CommandsConfiguration
            {
                RegisterDefaultCommandProcessors = false
            }
        );

        Bot = builder.Build();

        await Bot.ConnectAsync();

        _ = Task.Run( OnThink );

        TcpListener Server = new TcpListener( IPAddress.Any, 5000 );
        Server.Start();

        Thread DiscordToGameThread = new Thread( SendToGameServer );
        DiscordToGameThread.Start();

        while( true )
        {
            TcpClient Client = Server.AcceptTcpClient();

            lock(LockObj)Clients.Add( Client );

            Thread ClientThread = new Thread( () => HandleClient( Client ) );

            ClientThread.Start();
        }
    }

    private static async Task OnThink()
    {
        if( GameMessages.Count <= 0 )
        {
            await Task.Delay( TimeSpan.FromSeconds(1) );
            return;
        }

        DiscordGuild Guild = await Bot.GetGuildAsync( GUILD_ID );

        DiscordChannel Channel = await Guild.GetChannelAsync( FORUM_ID );

        if( Channel is not DiscordForumChannel Forum )
            return;

        DiscordThreadChannel? ForumThread = Forum.Threads.Where( t => t.Id == 1401679321776787516 ).FirstOrDefault();

        if( ForumThread is null )
            return;

        while( GameMessages.Count > 0 )
        {
            string DiscordMessage = GameMessages[0];
            GameMessages.RemoveAt(0);

            await ForumThread.SendMessageAsync( DiscordMessage );
        }

        await Task.Delay( TimeSpan.FromSeconds(1) );
    }

    public static async Task OnMessage( DiscordClient s, MessageCreatedEventArgs e )
    {
        if( e.Author.IsCurrent || e.Author.IsBot )
            return;

        if( string.IsNullOrEmpty( e.Message.Content ) )
            return;

        if( e.Guild.Id != GUILD_ID )
            return;

        if( e.Channel is not DiscordThreadChannel ForumPost )
            return;

        if( ForumPost.Parent is not DiscordForumChannel Forum )
            return;

        if( Forum.Id != FORUM_ID )
            return;

        DiscordMessages.Add( $"[Discord] {e.Author.Username}: {e.Message.Content}" );
        Console.WriteLine( $"[Discord] {e.Author.Username}: {e.Message.Content}\n channel: {ForumPost.Name} forum: {Forum.Name}" );
    }

    static void SendToGameServer()
    {
        while( true )
        {
            lock( LockObj )
            {
                if( DiscordMessages.Count > 0 )
                {
                    string DiscordMessage = DiscordMessages[0];
                    DiscordMessages.RemoveAt(0);

                    byte[] Buffer = Encoding.UTF8.GetBytes( DiscordMessage );

                    foreach( TcpClient Client in Clients.ToList() )
                    {
                        try
                        {
                            NetworkStream stream = Client.GetStream();
                            stream.Write( Buffer, 0, Buffer.Length );
                        }
                        catch
                        {
                            lock( LockObj )
                            {
                                Clients.Remove( Client );

                                try
                                {
                                    Client.Close();
                                }
                                catch { }
                            }
                        }
                    }
                }
            }
            Thread.Sleep(1000);
        }
    }

    static void HandleClient( TcpClient Client )
    {
        NetworkStream stream = Client.GetStream();
        byte[] Buffer = new byte[1024];

        while( true )
        {
            try
            {
                int bytes = stream.Read( Buffer, 0, Buffer.Length );

                if( bytes == 0 )
                    break;

                string message = Encoding.UTF8.GetString( Buffer, 0, bytes );
                GameMessages.Add( message );
            }
            catch
            {
                break;
            }
        }

        lock( LockObj )
        {
            Clients.Remove( Client );

            try
            {
                Client.Close();
            }
            catch { }
        }
    }
}
