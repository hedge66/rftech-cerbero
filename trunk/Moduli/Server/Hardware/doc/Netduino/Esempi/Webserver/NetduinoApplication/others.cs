using System;
using Microsoft.SPOT;
using MicroLiquidCrystal;
using Microsoft.SPOT.Hardware;
using SecretLabs.NETMF.Hardware.Netduino;

namespace NetduinoApplication
{
	public partial class Program
	{
		public static OutputPort p = new OutputPort(Pins.GPIO_PIN_D0, false);
		public static GpioLcdTransferProvider prov = new GpioLcdTransferProvider(Pins.GPIO_PIN_D8, Pins.GPIO_PIN_D9, Pins.GPIO_PIN_D10, Pins.GPIO_PIN_D11, Pins.GPIO_PIN_D12, Pins.GPIO_PIN_D13);
		public static Lcd lcd = new Lcd(prov);

		#region PASWD
		const string pwd = "c1c2c3c4";
		#endregion

		static void print(string s1, string s2)
		{
			lcd.SetCursorPosition(0, 0);
			lcd.Write(s1);
			lcd.SetCursorPosition(0, 1);
			lcd.Write(s2);
		}

		
	}
}
