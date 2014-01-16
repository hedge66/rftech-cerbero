using System;
using Microsoft.SPOT;
using System.Net.Sockets;
using System.Threading;
using System.Text;
using System.IO;
using System.Net;

namespace HttpServer
{
	public delegate void OnRequest(Request r);

	public class Request
	{
		public string request;
		public string RequestIOready;

		private event OnRequest ProcessRequest;
		private Socket client;
		private NetworkStream netstream;
		private Encoding ENCD = Encoding.UTF8;
	    
		public Request(Socket client, OnRequest requestdelegate)
		{
			this.ProcessRequest += requestdelegate;
			this.client = client;
		    new Thread(Process).Start();

		    Thread.Sleep(20);
		}

		private void Process()
		{
		    using (netstream = new NetworkStream(client))
		    {
		        byte[] data = new byte[2000];

		        int readbytes = 0;
		        while (netstream.DataAvailable)
		        {
		            readbytes += netstream.Read(data, readbytes, data.Length - readbytes);
		        }

		        string s = new string(ENCD.GetChars(data));

		        if (s != null && s.Substring(0, 3) == "GET")
		        {
		            request = s.Substring(4, s.IndexOf(' ', 4) - 4);
		            RequestIOready = Util.replace(request, '/', '\\');

		            Debug.Print(((IPEndPoint) client.RemoteEndPoint).Address.ToString() + " requested " + request);

		            ProcessRequest(this);

                    //Debug.GC(true);
                    //netstream.Close();
                    //netstream.Dispose();
                    //client.Close();
		        }

		    }

		}


		public void SendRaw(byte[] data)
		{
			netstream.Write(data, 0, data.Length);
		}
		public void SendRaw(string data)
		{
			SendRaw(ENCD.GetBytes(data));
		}

		public void Send(HtmlPage p)
		{
			SendRaw("HTTP/1.0 200 OK\r\nContent-Type: text/html; charset=utf-8\r\nContent-Length: " + p.ToString().Length.ToString() + "\r\n\r\n" + p.ToString());
		}

		public void Send(string path)
		{
			try
			{
				path = ((path[0] == '\\') ? "\\SD" : "\\SD\\" ) + path;
				if (File.Exists(path))
				{
					byte[] header = ENCD.GetBytes("HTTP/1.0 200 OK\r\nContent-Type: " + Util.GetMime(path) + "\r\nContent-Length: " + new FileInfo(path).Length.ToString() + "\r\n\r\n");
					SendRaw(header);
					SendRaw(File.ReadAllBytes(path));
				}
				else
				{
					if (Directory.Exists(path))
					{
						HtmlPage p = new HtmlPage("<title>Netduino: " + path + "</title>", "Directories in " + path + " at " + DateTime.Now.ToString() + "<hr>");
						string[] directories = Directory.GetDirectories(path);
						string[] files = Directory.GetFiles(path);

						foreach (string dir in directories)
						{
							p.body += "<a href = \"" + Util.replace(dir, '\\', '/').Substring(4) + "\">" + dir.Substring(dir.LastIndexOf('\\') + 1) + "</a><br>";
						}
						p.body += "Files in " + path + " at " + DateTime.Now.ToString() + "<hr>";

						foreach (string file in files)
						{
							p.body += "<a href = \"" + Util.replace(file, '\\', '/').Substring(4) + "\">" + file.Substring(file.LastIndexOf('\\') + 1) + "        Size: " + new FileInfo(file).Length.ToString() + "bytes </a><br>";
						}
						Send(p);
					}
					else
						SendRaw("HTTP/1.0 404 Not Found\r\n\r\n");
				}
			}
			catch { SendRaw("Exception"); }

		}
	}
	
}
