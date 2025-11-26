#include <iostream>
#include <file/archive/archive_loader.hpp>
#include <string>
#include <filesystem>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <sstream>
#include <fstream>

using PameECS::File::Archive::ArchiveLoader;

int main() {
	std::cout << "PEAC Archive file path: ";

	std::filesystem::path filepath;
	std::cin >> filepath;

	auto threadPool = std::make_shared<BS::thread_pool<0U>>();
	auto consoleSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
	auto fileSink = std::make_shared<spdlog::sinks::basic_file_sink_mt>("PEAC_TEST.log", true);
	std::vector<spdlog::sink_ptr> sinks{ consoleSink, fileSink };
	auto logger = std::make_shared<spdlog::logger>("PEAC_tester", sinks.begin(), sinks.end());
	consoleSink->set_level(spdlog::level::trace);
	fileSink->set_level(spdlog::level::trace);
	logger->set_level(spdlog::level::trace);

	logger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%n] [%^level %L%$] [thread %t] %v");

	try {
		ArchiveLoader loader(filepath, threadPool, logger);
		while (true) {
			std::string command;
			std::cin >> command;
			if (command == "exit") {
				break;
			}
			else if (command == "extract") {
				std::string path, writePath;
				std::cin >> path >> writePath;
				auto data = loader.GetFileData(path);
				std::cout << "Size: " << data.size() << std::endl;
				std::ofstream ofs(writePath, std::ios::binary);
				ofs.write(reinterpret_cast<const char*>(data.data()), data.size());
			}
		}
	}
	catch (const Pame::Exceptions::ExceptionBase& e) {
		std::stringstream ss;
		ss << e.GetTrace();
		logger->critical("{}: {}\n{}", e.GetExceptionTypeName(), e.what(), ss.str());
	}
}
