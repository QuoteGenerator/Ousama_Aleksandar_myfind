#include <iostream>
#include <vector>
#include <sys/stat.h>
#include <unistd.h>
#include <filesystem>
#include <strings.h>
#include <cstring>


void pathSearcher(char *searchPath, const std::string& inputFilename, std::vector<std::string>& foundPaths, std::vector<std::string>& foundFileNames, bool caseInsensitve, bool recursiveOption){
    if(recursiveOption == true){
        for(const auto& entry : std::filesystem::recursive_directory_iterator(searchPath)){
            const std::string searchedFilename = entry.path().filename().string();
            if(!std::filesystem::is_regular_file(entry)){
                continue;
            }
            if(caseInsensitve == true){
                if(strncasecmp(searchedFilename.c_str(), inputFilename.c_str(), inputFilename.length()) == 0){
                    foundFileNames.emplace_back(searchedFilename);
                    foundPaths.emplace_back(std::filesystem::absolute(entry));
                }
            }
            else if(strncmp(searchedFilename.c_str(), inputFilename.c_str(), inputFilename.length()) == 0){
                foundFileNames.emplace_back(searchedFilename);
                foundPaths.emplace_back(std::filesystem::absolute(entry));
            } 
        }
    }
    else{
        for(const auto& entry : std::filesystem::directory_iterator(searchPath)){
            const std::string searchedFilename = entry.path().filename().string();
            if(!std::filesystem::is_regular_file(entry)){
                continue;
            }
            if(caseInsensitve == true){
                if(strncasecmp(searchedFilename.c_str(), inputFilename.c_str(), inputFilename.length()) == 0){
                    foundFileNames.emplace_back(searchedFilename);
                    foundPaths.emplace_back(std::filesystem::absolute(entry));
                }
            }
            else if(strncmp(searchedFilename.c_str(), inputFilename.c_str(), inputFilename.length()) == 0){
                foundFileNames.emplace_back(searchedFilename);
                foundPaths.emplace_back(std::filesystem::absolute(entry));
            }   
        }
            
    }
}

int main(int argc, char *argv[]){

    std::vector<std::string> foundPaths;
    std::vector<std::string> foundFileNames;
    int opt; 
    bool caseInsensitve = true, recursiveOption = true;
    /*while((opt = getopt(argc, argv, "iR")) != -1){

    } */

    char* searchDir = argv[3];
    std::string inputFilname = argv[4];

    pathSearcher(searchDir, inputFilname, foundPaths, foundFileNames, caseInsensitve, recursiveOption);

    for(int i = 0; i < foundFileNames.size(); ++i){
        std::cout << foundFileNames[i]<< ": " << foundPaths[i] <<'\n';
    }
}

