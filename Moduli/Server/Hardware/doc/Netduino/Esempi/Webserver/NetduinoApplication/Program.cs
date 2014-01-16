using System;
using System.Net;
using System.Net.Sockets;
using System.Threading;
using Microsoft.SPOT;
using Microsoft.SPOT.Hardware;
using SecretLabs.NETMF.Hardware.Netduino;
using HttpServer;

namespace NetduinoApplication
{
	public partial class Program
	{
		private static AnalogInput i = new AnalogInput(Cpu.AnalogChannel.ANALOG_0);
		private static OutputPort led = new OutputPort(Pins.ONBOARD_LED, false);

		public static void Main()
		{
			//NetworkInterface.GetAllNetworkInterfaces()[0].EnableStaticIP("192.168.2.113", "255.255.255.0", "192.168.2.1");
		    Utility.SetLocalTime(new DateTime(2013, 2, 11, 9, 43, 0)); //(GetNtpTime("213.ip-net02.empolis.com", +1));
			Debug.Print(DateTime.Now.ToString());
			WebServer s = new WebServer(Process);
			led.Write(true);
			Thread.Sleep(100);
			led.Write(false);
			new Thread(s.Start).Start();
			Thread.Sleep(-1);
		}

		private static double Temperature(int measurements)
		{
			double temp = 0D;

			for (int x = 0; x < measurements; x++)
			{
				temp += i.Read();
			}
			return temp / measurements * 330D;
		}


		static void Process(Request r)
		{
			switch(r.request)
			{
				case "/temp":
				case "/temp/":
					r.Send(new HtmlPage("<title>Temperature</title>", Temperature(8000).ToString() + "°C"));
					break;
				case "/led":
				case "/led/":
					r.Send(new HtmlPage("<title>Led</title>", (!led.Read()).ToString()));
					led.Write(!led.Read());
					break;

				default:
					r.Send(r.RequestIOready);
				break;
			}
		}


		private static DateTime GetNtpTime(String timeServer, int timeZoneOffset)
		{
			var ep = new IPEndPoint(Dns.GetHostEntry(timeServer).AddressList[0], 123);
			var ntpData = new byte[48];
			using (var s = new Socket(AddressFamily.InterNetwork, SocketType.Dgram, ProtocolType.Udp))
			{
				s.SendTimeout = s.ReceiveTimeout = 10000;
				s.Connect(ep);
				ntpData[0] = 0x1B;
				s.Send(ntpData);
				s.Receive(ntpData);
				s.Close();
			}
			const byte offsetTransmitTime = 40;
			ulong intpart = 0;
			ulong fractpart = 0;
			for (var i = 0; i <= 3; i++)
				intpart = (intpart << 8) | ntpData[offsetTransmitTime + i];
			for (var i = 4; i <= 7; i++)
				fractpart = (fractpart << 8) | ntpData[offsetTransmitTime + i];
			ulong milliseconds = (intpart * 1000 + (fractpart * 1000) / 0x100000000L);
			var timeSpan = TimeSpan.FromTicks((long)milliseconds * TimeSpan.TicksPerMillisecond);
			var dateTime = new DateTime(1900, 1, 1);
			dateTime += timeSpan;
			var offsetAmount = new TimeSpan(timeZoneOffset, 0, 0);
			var networkDateTime = (dateTime + offsetAmount);
			return networkDateTime;
		}

	}
}
