using System.Text;

namespace HttpServer
{
	public class Util
	{
		public static string GetMime(string filename)
		{
			if (filename[filename.Length - 1] == '\\')
				filename = filename.Substring(0, filename.Length - 1);
			switch (filename.Substring(filename.LastIndexOf('.') + 1).ToLower())
			{
				case "html":
				case "htm":
					return "text/html; charset=utf-8";
				case "jpg":
				case "jpeg":
					return "image/jpeg";
				case "png":
					return "image/png";
				case "gif":
					return "image/gif";
				case "ico":
					return "image/png";
				default:
					return "unknown/unknown";
			}
		}

		public static string replace(string s, char charold, char charnew)
		{
			StringBuilder b = new StringBuilder(s);
			b.Replace(charold, charnew);
			return b.ToString();
		}
	}
}
