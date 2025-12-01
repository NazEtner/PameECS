#include <iostream>
#include <file/archive/archive_creator.hpp>
#include <exceptions/exception_base.hpp>
#include <sstream>

using PameECS::File::Archive::ArchiveCreator;

int main() {
	int returnCode = 0;
	try {
		ArchiveCreator creator;
		while (true) {
			std::string command;
			std::cin >> command;
			if (command == "add_file") {
				std::string path;
				std::cin >> path;
				creator.AddFileToVirtualRoot(path);
			}
			else if (command == "add_dir") {
				std::string path;
				std::cin >> path;
				creator.AddDirectoryToVirtualRoot(path);
			}
			else if (command == "write") {
				std::string path;
				std::cin >> path;
				creator.Write(path);
			}
			else if (command == "exit") {
				break;
			}
			else {
				std::cout << command << " is not valid command." << std::endl;
				std::cout << "\t" << "add_file {filename}" << std::endl;
				std::cout << "\t" << "add_dir {filename}" << std::endl;
				std::cout << "\t" << "write {filename}" << std::endl;
				std::cout << "\t" << "exit" << std::endl;
			}
		}
	}
	catch (const Pame::Exceptions::ExceptionBase& e) {
		std::cerr << e.GetExceptionTypeName() << " : " << e.what() << std::endl;
		std::cerr << e.GetTrace() << std::endl;
		returnCode = -1;
	}
	return returnCode;
}
