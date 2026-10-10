#include <iostream>
#include <vector>
#include <unistd.h>
#include <filesystem>
#include <strings.h>
#include <sys/wait.h>


void pathSearcher(char *searchPath, const std::string& inputFilename, std::vector<std::string>& foundPaths, std::vector<std::string>& foundFileNames, bool caseInsensitve, bool recursiveOption){
    if(recursiveOption == true){
        for(const auto& entry : std::filesystem::recursive_directory_iterator(searchPath)){
            const std::string searchedFilename = entry.path().filename().string();
            if(!std::filesystem::is_regular_file(entry)){
                continue; //überspringe Ordner.
            }
            
            if (searchedFilename.length() != inputFilename.length())
            {
                continue; //automatisch falsch
            }
            
            if(caseInsensitve == true){
                if(strncasecmp(searchedFilename.c_str(), inputFilename.c_str(), inputFilename.length()) == 0){
                    foundFileNames.emplace_back(searchedFilename);
                    foundPaths.emplace_back(std::filesystem::absolute(entry));
                }
            }
            else if (inputFilename == searchedFilename){
                {
                    foundFileNames.emplace_back(searchedFilename);
                    foundPaths.emplace_back(std::filesystem::absolute(entry));
                }
            } 
        }
    }
    else{
        for(const auto& entry : std::filesystem::directory_iterator(searchPath)){
            const std::string searchedFilename = entry.path().filename().string();
            if(!std::filesystem::is_regular_file(entry)){
                continue;
            }
            
            if (searchedFilename.length() != inputFilename.length())
            {
                continue; //automatisch falsch
            }
            
            if(caseInsensitve == true){                
                if(strncasecmp(searchedFilename.c_str(), inputFilename.c_str(), inputFilename.length()) == 0){
                    foundFileNames.emplace_back(searchedFilename);
                    foundPaths.emplace_back(std::filesystem::absolute(entry));
                }
            } else if (inputFilename == searchedFilename){
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
    bool caseInsensitve = false, recursiveOption = false; //!!!Unübersichtlich, wenn seitlich (bei wichtigen daten lieben nicht lg aleks)!
    
    
    
    while((opt = getopt(argc, argv, "iR")) != -1){
        switch (opt)
        {
            case 'i':
                caseInsensitve = true;
            break;
        
            case 'R':
                recursiveOption = true;
            break;
            
            default: break;
        }
    }

    char* searchDir = argv[optind];
    std::string inputFilename; // Wird in der for schleife zugewiesen (direkt darunter)
    
    for (int i = optind+1; i < argc; i++){
        pid_t PID = fork();
        
        if (PID == -1)
        {
            break; //Fehler beim erzeugen des Kindprozesses
            
        } else if (PID == 0)
        {
            inputFilename = argv[i];
            pathSearcher(searchDir, inputFilename, foundPaths, foundFileNames, caseInsensitve, recursiveOption);
        } else
        {
            _exit(0); // Thread safe
        }
    }
    while (waitpid(-1, nullptr, 0) > 0) {
        
    }
    
    for(std::size_t i = 0; i < foundFileNames.size(); ++i){
        std::cout << foundFileNames[i]<< ": " << foundPaths[i] <<'\n';
    }
}

