using System.Net;
using System.Net.Sockets;
using System.Text;

using Newtonsoft.Json;
using Newtonsoft.Json.Linq;

using DSharpPlus;
using DSharpPlus.Entities;
using DSharpPlus.EventArgs;
using DSharpPlus.Commands;
using DSharpPlus.Commands.Processors.TextCommands;
using DSharpPlus.Commands.Processors.TextCommands.Parsing;

public class GameServer( string ID, string Name, ulong Channel )
{
    public readonly string ID = ID;
    public readonly string Name = Name;
    public readonly ulong Channel = Channel;

    public List<string> DiscordMessages = new();

    public TcpClient? client;
};

public static class Program
{
// Non-nullable field must contain a non-null value when exiting constructor.
#pragma warning disable CS8618
    private static DiscordClient Bot;
    private static ulong GUILD_ID;
#pragma warning restore CS8618

    private static string? Prefix;
    private static CommandsExtension? Commands;

    private static List<GameServer> Servers = new List<GameServer>();

    private static List<TcpClient> Clients = new List<TcpClient>();
    private static object LockObj = new object();

    public static async Task Main( string[] args )
    {
        try
        {
            string ConfigFile = Path.Combine( AppContext.BaseDirectory, "config", "config.json" );

            JObject? ConfigContext = JsonConvert.DeserializeObject<JObject>( File.ReadAllText( ConfigFile ) );

// Anything wrong in this statement is the user's fault.
#pragma warning disable CS8602 // Dereference of a possibly null reference.
#pragma warning disable CS8604 // Possible null reference argument.

            GUILD_ID = (ulong)ConfigContext.GetValue( "guild" );

            DiscordClientBuilder builder = DiscordClientBuilder.CreateDefault(
                File.ReadAllText(
                    Path.Combine( AppContext.BaseDirectory, "config", ConfigContext[ "token_file" ].ToString() )
                ),
                DiscordIntents.GuildMessages | DiscordIntents.MessageContents
            );

            builder.ConfigureEventHandlers( b => b
                .HandleMessageCreated( OnMessage )
            );

            foreach( JObject? srv in ConfigContext[ "servers" ] )
            {
                Servers.Add( new GameServer(
                    srv[ "ID" ].ToString(),
                    srv[ "name" ].ToString(),
                    (ulong)srv[ "channel" ]
                ) );
            }

            Prefix = ConfigContext[ "prefix" ]?.ToString();

            if( string.IsNullOrEmpty( Prefix ) )
            {
                Console.WriteLine( "No valid bot prefix given. commands will be disabled." );
            }
            else
            {
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
            }
#pragma warning restore CS8602 // Dereference of a possibly null reference.
#pragma warning restore CS8604 // Possible null reference argument.

            Bot = builder.Build();

            await Bot.ConnectAsync();
        }
        catch( Exception e )
        {
            Console.WriteLine( $"Critical error {e.GetType()}: {e.Message}\nStack Trace:\n{e.StackTrace}" );
            Environment.Exit(1);
        }

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

#pragma warning disable CS1998 // Async method lacks 'await' operators and will run synchronously
    public static async Task OnMessage( DiscordClient s, MessageCreatedEventArgs e )
#pragma warning restore CS1998 // Async method lacks 'await' operators and will run synchronously
    {
        if( e.Author.IsCurrent || e.Author.IsBot )
            return;

        if( string.IsNullOrEmpty( e.Message.Content ) )
            return;

        if( e.Guild.Id != GUILD_ID )
            return;

        GameServer? server = Servers.Where( srv => srv.Channel == e.Channel.Id ).FirstOrDefault();

        if( server is null )
            return;

        server.DiscordMessages.Add( $"[Discord] {e.Author.Username}: {e.Message.Content}\n" );
    }

    static void SendToGameServer()
    {
        while( true )
        {
            foreach( GameServer server in Servers )
            {
                if( server.client is null )
                    continue;

                if( server.DiscordMessages.Count > 0 )
                {
                    NetworkStream stream = server.client.GetStream();

                    lock( LockObj )
                    {
                        string DiscordMessage = server.DiscordMessages[0];
                        server.DiscordMessages.RemoveAt(0);

                        byte[] Buffer = Encoding.UTF8.GetBytes( DiscordMessage );
                        stream.Write( Buffer, 0, Buffer.Length );
                    }
                }
            }
            Thread.Sleep(1000);
        }
    }

    static void HandleClient( TcpClient Client )
    {
        NetworkStream stream = Client.GetStream();
        byte[] Buffer = new byte[8192];

        while( true )
        {
            try
            {
                int bytes = stream.Read( Buffer, 0, 8192 );

                if( bytes == 0 )
                    break;

                string message = Encoding.UTF8.GetString( Buffer, 0, bytes );

                GameServer? server = Servers.Where( srv => srv.client is not null && srv.client == Client ).FirstOrDefault();

                if( server is null )
                {
                    if( message.StartsWith( "login" ) )
                    {
                        string ID = message[6..];
                        server = Servers.Where( srv => srv.ID == ID ).FirstOrDefault();

                        if( server is not null )
                        {
                            message = $"Server \"{ID}\" online";
                            server.client = Client;
                        }
                    }
                }

                if( server is null )
                {
                    byte[] Tell = Encoding.UTF8.GetBytes( $"{{ \"error\": \"Server ID \\\"{message}\\\" not found on C#'s config context.\"}}" );
                    stream.Write( Tell, 0, Tell.Length );
                    break;
                }

                _ = Task.Run( async () =>
                {
                    DiscordGuild Guild = await Bot.GetGuildAsync( GUILD_ID );
                    DiscordChannel Channel = await Guild.GetChannelAsync( server.Channel );
                    await Channel.SendMessageAsync( message );
                } );
            }
            catch
            {
                break;
            }
        }
        RemoveClient( Client );
    }

    private static void RemoveClient( TcpClient Client )
    {
        lock( LockObj )
        {
            foreach( GameServer server in Servers )
            {
                if( server.client is not null && server.client == Client )
                {
                    server.client = null;
                }
            }

            Clients.Remove( Client );

            try
            {
                Client.Close();
            }
            catch { }
        }
    }
}
