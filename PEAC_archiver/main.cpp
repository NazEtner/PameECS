#include <iostream>
#include <file/archive/archive_creator.hpp>
#include <exceptions/exception_base.hpp>
#include <sstream>

using PameECS::File::Archive::ArchiveCreator;

int main() {
	try {
		ArchiveCreator creator;
		int returnCode = 0;
		while (true) {
			std::string command, param;
			std::cin >> command >> param;
			if (command == "add_file") {
				creator.AddFileToVirtualRoot(param);
			}
			else if (command == "add_dir") {
				creator.AddDirectoryToVirtualRoot(param);
			}
			else if (command == "write") {
				creator.Write(param);
			}
			else if (command == "exit") {
				auto ss = std::istringstream(param);
				ss >> returnCode;
				break;
			}
			else {
				std::cout << command << " is not valid command." << std::endl;
				std::cout << "\t" << "add_file {filename}" << std::endl;
				std::cout << "\t" << "add_dir {filename}" << std::endl;
				std::cout << "\t" << "write {filename}" << std::endl;
				std::cout << "\t" << "exit {returncode}" << std::endl;
			}
		}

		return returnCode;
	}
	catch (const Pame::Exceptions::ExceptionBase& e) {
		std::cerr << e.GetExceptionTypeName() << " : " << e.what() << std::endl;
		std::cerr << e.GetTrace() << std::endl;
	}
}
