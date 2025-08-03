using System.Net;
using System.Net.Sockets;
using System.Text;

public class Program
{
    private static List<TcpClient> Clients = new List<TcpClient>();
    private static object LockObj = new object();

    private static List<string> DiscordMessages = new();

    public static void Main()
    {
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

                Console.WriteLine( message );

                // Test momentario
                DiscordMessages.Add( message );
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
