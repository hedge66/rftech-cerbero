namespace HttpServer
{
	public class HtmlPage
	{
		public string header, body;

		public HtmlPage(string header, string body)
		{
			this.header = header;
			this.body = body;
		}
		public override string ToString()
		{
			return "<!DOCTYPE html><html><head>" + header + "</head><body>" + body + "</body></html>";
		}
	}
}
