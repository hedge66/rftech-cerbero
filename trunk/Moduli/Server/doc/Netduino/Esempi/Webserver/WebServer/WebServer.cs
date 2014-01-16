using System.Net;
using System.Net.Sockets;

namespace HttpServer
{
	public class WebServer
	{
		OnRequest requestdelegate;

		public WebServer(OnRequest requestdelegate)
		{
			this.requestdelegate = requestdelegate;
		}

		public void Start()
		{
		    using (var listener = new Socket(AddressFamily.InterNetwork, SocketType.Stream, ProtocolType.Tcp))
		    {
		        listener.Bind(new IPEndPoint(IPAddress.Any, 80));
		        listener.Listen(1);

		        while (true)
		        {
		            using (var connection = listener.Accept())
		            {
                        var request = new Request(connection, requestdelegate);    
		            }
		            
		        }
		    }
		}
	}
}
