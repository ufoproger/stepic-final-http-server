#include <iostream>
#include <unistd.h>
#include <sys/types.h>
#include <cstdlib>
#include <boost/bind.hpp>
#include <boost/smart_ptr.hpp>
#include <boost/asio.hpp>
#include <boost/thread/thread.hpp>
#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>
#include <boost/regex.hpp>
#include <fstream>
#include <streambuf>

using boost::asio::ip::tcp;
namespace po = boost::program_options;

const int max_length = 65536;

//std::string directory;
boost::filesystem::path directory;

typedef boost::shared_ptr<boost::asio::ip::tcp::socket> socket_ptr;
/*
std::string urlencode(const std::string &s)
{
std::string escaped="";
int max = c.length();
for(int i=0; i<max; i++)
{
if ( (48 <= c[i] && c[i] <= 57) ||//0-9
(65 <= c[i] && c[i] <= 90) ||//abc...xyz
(97 <= c[i] && c[i] <= 122) || //ABC...XYZ
(c[i]=='~' || c[i]=='!' || c[i]=='*' || c[i]=='(' || c[i]==')' || c[i]=='\'')
)
{
escaped.append( &c[i], 1);
}
else
{
escaped.append("%");
escaped.append( char2hex(c[i]) );//converts char 255 to string "ff"
}
}

wstring escaped_w = L"";

wchar_t w[1024];
mbstowcs((wchar_t*)&w, escaped.c_str(), 1024);

escaped_w = wstring(w);

return escaped_w;
}
*/
// Функция URL-декодирования.
// Функция преобразует строку данных st в нормальное представление.
// Результат помещается в ту же строку, что была передана в параметрах.
std::string urldecode(std::string str)
{
	char hex[3];
	int code;

	std::string result;

	for (auto it = str.begin(); it != str.end(); ++it)
	{
		switch (*it)
		{
			case '%':
			{
				hex[0] = *(++it);
				hex[1] = *(++it);
				hex[2] = 0;

				sscanf(hex, "%X", &code);

				result.push_back((char)code);

				break;
			}

			case '+':
			{
				result.push_back(' ');

				break;
			}

			default:
				result.push_back(*it);
		}
	}

	return result;
}

std::string process_request(std::string s)
{
	boost::regex regex("^GET (\\S+) HTTP");
	boost::smatch result;

	if (!boost::regex_search(s.cbegin(), s.cend(), result, regex))
		return "error not match";

	std::string file(result[1]);

	std::cout << (directory / file) << std::endl;

	file = file.substr(0, file.find_first_of('?'));
	file = urldecode(file);

	std::cout << (directory / file) << std::endl;

	if (!boost::filesystem::exists(directory / file))
		return "HTTP/1.0 404 Not Found\r\n\r\n";

	std::string response("HTTP/1.0 200 OK\r\n");

	response += "Connection: close\r\n";

	std::ifstream fin((directory / file).string());
	std::string content((std::istreambuf_iterator<char>(fin)), std::istreambuf_iterator<char>());

	response += "Content-Type: text/html\r\n";
	response += "Content-Length: " + std::to_string(content.size()) + "\r\n";
	response += "\r\n" + content;

	return response;
}

void session(socket_ptr sock)
{
	try
	{
		char data[max_length];
		boost::system::error_code error;

		size_t length = sock->read_some(boost::asio::buffer(data), error);

		if (error == boost::asio::error::eof)
			return;
		else if (error)
			throw boost::system::system_error(error);

		std::string s(data);

		std::cout << "received = '" << s << "'" << std::endl;

		s = process_request(s);
		std::cout << "response = '" << s << "'" << std::endl;

		boost::asio::write(*sock, boost::asio::buffer(s));
	}
	catch (std::exception& e)
	{
		std::cerr << "Exception in thread: " << e.what() << "\n";
	}
}

void server(boost::asio::io_service& io_service, std::string host, short port)
{
	tcp::acceptor a(io_service, tcp::endpoint(boost::asio::ip::address::from_string(host), port));

	for (;;)
	{
		//std::cout << "Ожидаю содинение" << std::endl;
		socket_ptr sock(new tcp::socket(io_service));
		a.accept(*sock);
		boost::thread t(boost::bind(session, sock));
	}
}

pid_t daemonize()
{
	pid_t pid = fork();

	if (pid)
	{
		std::cout << "Создан демон с pid = " << pid << "." << std::endl;
		_exit(0);
	}

	setsid();

	return pid;
}

int main(int argc, char* argv[])
{
	po::options_description desc("Параметры сервера");

	desc.add_options()
		("help", "Этот вывод")
		("port,p", po::value<int>(), "Порт")
		("host,h", po::value<std::string>(), "Адрес")
		("directory,d", po::value<std::string>(), "Корневая директория")
	;

	po::variables_map vm;
	po::store(po::parse_command_line(argc, argv, desc), vm);
	po::notify(vm);

	if (vm.count("help")) {
		std::cout << desc << "\n";
		return 1;
	}

	int port = 0;

	if (vm.count("port"))
	{
		port = vm["port"].as<int>();
	}
	else
	{
		std::cout << "Порт не выбран!" << std::endl;
		return 1;
	}

	std::string host;

	if (vm.count("host"))
	{
		host = vm["host"].as<std::string>();
	}
	else
	{
		std::cout << "Адрес не выбран!" << std::endl;
		return 1;
	}

	if (vm.count("directory"))
	{
		directory = vm["directory"].as<std::string>();
	}
	else
	{
		std::cout << "Директория не выбрана!" << std::endl;
		return 1;
	}

	if (!boost::filesystem::exists(directory))
	{
		std::cout << "Директория не существует!" << std::endl;
		return 1;
	}

	std::cout << "\"" << directory << "\"" << " -> " << host << ":" << port << "." << std::endl;

	daemonize();
	//std::cout << "result = '" << process_request("GET test+%48%D0%BF%D1%80%D0%B8%D0%B2%D0%B5%D1%82.html?123 HTTP/1.0\r\n") << "'" << std::endl;
	//return 0;

	try
	{
		boost::asio::io_service io_service;
		server(io_service, host, port);
	}
	catch (std::exception& e)
	{
		std::cerr << "Exception: " << e.what() << "\n";
	}

	return 0;
}