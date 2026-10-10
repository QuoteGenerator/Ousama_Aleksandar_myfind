#include <iostream>
#include <vector>
#include <unistd.h>
#include <filesystem>
#include <strings.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <limits.h>
#include <string>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <cerrno>

void printUsage(char* programName){
    std::cerr << "Usage: " << programName << " [-R] [-i] searchpath filename1 [filename2] ...[filenameN]";
    exit(EXIT_FAILURE);
}


void pathSearcher(char *searchPath, const std::string& inputFilename, std::vector<std::string>& foundPaths, std::vector<std::string>& foundFileNames, bool caseInsensitive, bool recursiveOption){
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
            
            if(caseInsensitive == true){
                if(strncasecmp(searchedFilename.c_str(), inputFilename.c_str(), inputFilename.length()) == 0){
                    foundFileNames.emplace_back(searchedFilename);
                    foundPaths.emplace_back(std::filesystem::absolute(entry));
                }
            }
            else if (searchedFilename == inputFilename){
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
            
            if (searchedFilename.length() != inputFilename.length())
            {
                continue; //automatisch falsch
            }
            
            if(caseInsensitive == true){                
                if(strncasecmp(searchedFilename.c_str(), inputFilename.c_str(), inputFilename.length()) == 0){
                    foundFileNames.emplace_back(searchedFilename);
                    foundPaths.emplace_back(std::filesystem::absolute(entry));
                }
            } else if (searchedFilename == inputFilename){
                foundFileNames.emplace_back(searchedFilename);
                foundPaths.emplace_back(std::filesystem::absolute(entry));
            }   
        }
    }
}

void writeIntoPipe(int fileDescriptor, const std::vector<std::string>& foundPaths, const std::vector<std::string>& foundFilenames){
    FILE* writing;
    if((writing = fdopen(fileDescriptor, "w")) == NULL){
        std::cerr << "Konnte nicht schreiben\n";
        exit(EXIT_FAILURE);
    }

    if(foundFilenames.empty() || foundPaths.empty()){
        std::fprintf(writing, "%d: File not found\n", (int)getpid());
        std::fflush(writing);
        fclose(writing);
        return;
    }

    for(size_t i = 0; i < foundFilenames.size(); ++i){
        std::fprintf(writing, "%d: %s: %s\n", (int)getpid(), foundFilenames[i].c_str(), foundPaths[i].c_str());
        std::fflush(writing);
    }
    fclose(writing);
}

int main(int argc, char *argv[]){

    std::vector<std::string> foundPaths;
    std::vector<std::string> foundFileNames;
    int opt; 
    bool caseInsensitive = false, recursiveOption = false; //!!!Unübersichtlich, wenn seitlich (bei wichtigen daten lieben nicht lg aleks)!
    bool optionR = false, optionI = false, error = false;
    while((opt = getopt(argc, argv, "iR")) != -1){
        switch (opt)
        {
            case 'i':
                if(optionI){
                    error = true;
                    break;
                }
                caseInsensitive = true;
                optionI = true;
                break;
            case 'R':
                if(optionR){
                    error = true;
                    break;
                }
                recursiveOption = true;
                optionR = true;
                break;
            
            default: 
                error = true;
                break;
        }
    }

    if(error){
        printUsage(argv[0]);
    }

    if(argc <= optind + 1){
        printUsage(argv[0]);
    }

    int fileDescriptor[2];
    char buffer[PIPE_BUF];
    FILE* reading;
    char* searchDir = argv[optind];
    std::string inputFilename; // Wird in der for schleife zugewiesen (direkt darunter)

    if (!std::filesystem::exists(searchDir) || !std::filesystem::is_directory(searchDir)) {
        std::cerr << "Invalid search directory: " << searchDir << '\n';
        return EXIT_FAILURE;
    }

    if(pipe(fileDescriptor) < 0){
        std::cerr <<"pipe\n";
        return EXIT_FAILURE;
    }

    
    for (int i = optind+1; i < argc; i++){
        pid_t PID = fork();
        
        if (PID == -1)
        {
            std::cerr << "Ein Fehler ist aufgetreten, beim erstellen eines Kindprozesses!";
            pid_t result;
            while ((result = waitpid(-1, nullptr, 0)) > 0) {
                //wait for children, should be empty
            }

            if(result == -1 && errno != ECHILD){
                std::cerr << "waitpid\n";
            }
            return EXIT_FAILURE;
        } 
        else if (PID == 0)
        {
            close(fileDescriptor[0]);
            inputFilename = argv[i];
            pathSearcher(searchDir, inputFilename, foundPaths, foundFileNames, caseInsensitive, recursiveOption);
            writeIntoPipe(fileDescriptor[1], foundPaths, foundFileNames);
            return EXIT_SUCCESS;
        }
    }
    
    close(fileDescriptor[1]);

    if((reading = fdopen(fileDescriptor[0], "r")) == NULL){
        std::cerr << "Konnte nicht lesen\n";
        return EXIT_FAILURE;
    }

    while(fgets(buffer, PIPE_BUF, reading) != NULL){
        fputs(buffer, stdout);
        fflush(stdout);
    }

    fclose(reading);

    pid_t result;
    while ((result = waitpid(-1, nullptr, 0)) > 0) {
        //wait for children, should be empty
    }

    if(result == -1 && errno != ECHILD){
        std::cerr << "waitpid\n";
        return EXIT_FAILURE;
    }
}

