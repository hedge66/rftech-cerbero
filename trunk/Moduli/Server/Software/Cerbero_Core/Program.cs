using System;
using System.Net;
using System.Net.Sockets;
using System.Threading;
using Microsoft.SPOT;
using Microsoft.SPOT.Hardware;
using SecretLabs.NETMF.Hardware;
using SecretLabs.NETMF.Hardware.NetduinoPlus;
using Microsoft.SPOT.Net.NetworkInformation;
using Microsoft.SPOT.Net;
using System.Text;

namespace Cerbero_Core
{
    public class Program
    {
        public static void Main()
        {
            // write your code here
            OutputPort led = new OutputPort(Pins.ONBOARD_LED, false);
            // write your code here
            Thread thread1 = new Thread(new ThreadStart(Listener));
            thread1.Start();
            while (true)
            {
            }
        }

        public static void Listener()
        {
            // write your code here
            string ip = "192.168.0.6";
            int port = 9099;
            OutputPort led = new OutputPort(Pins.ONBOARD_LED, false);
            string rawData = "";
            bool status;
            NetworkInterface networkInterface = NetworkInterface.GetAllNetworkInterfaces()[0];
            networkInterface.EnableStaticIP(ip, "255.255.255.0", "192.168.0.1");

            using (System.Net.Sockets.Socket socket = new Socket(AddressFamily.InterNetwork, SocketType.Stream, ProtocolType.Tcp))
            {
                socket.Bind(new IPEndPoint(IPAddress.Parse(ip), port));
                Debug.Print("Bind the socket client to " + ip);
                socket.Listen(1);

                while (true)
                {
                    using (Socket currentSocket = socket.Accept())
                    {
                        bool keepAlive = true;
                        while (keepAlive)
                        {

                            if (currentSocket.Poll(-1, SelectMode.SelectRead))
                            {
                                byte[] bytes = new byte[currentSocket.Available];
                                int count = currentSocket.Receive(bytes);
                                string request = new String(Encoding.UTF8.GetChars(bytes));
                                if (request == "start")
                                {
                                    currentSocket.Send(Encoding.UTF8.GetBytes("\r\n" + "Start Engine"));
                                    ; keepAlive = false;

                                }
                                else if (request == "stop")
                                {
                                    currentSocket.Send(Encoding.UTF8.GetBytes("\r\n" + "You ask for a c"));
                                    ; keepAlive = false;

                                }
                                Debug.Print("Request " + request);
                            }
                        }
                        Debug.Print("Server Close");
                    }
                }
            }
         }
    }
}
