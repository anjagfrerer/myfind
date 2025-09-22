#include <iostream>
#include <unistd.h>
#include <filesystem>
#include <vector>
#include <dirent.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <cstring>
using namespace std;
namespace fs = std::filesystem;

string lowerCase(string characters) {
    for(int i=0; i < characters.size(); i++) {
        //static_cast<unsigned char> ... stellt sicher, dass es als gültiger positiver Wert an tolower() übergeben wird
        characters[i] = tolower(static_cast<unsigned char>(characters[i]));
    }
    return characters;
}

bool search(string sp, string filename, bool recursive, bool casesensitive) {
    const char* searchpath = sp.c_str();
    DIR *dir = opendir(searchpath);
    struct dirent *entry; // direkt ... vordefinierte Struktur aus <dirent.h>, die Informationen über einen Eintrag in einem Verzeichnis speichert (z. B. Dateinamen)
    bool fileFound = false;
    // kann das Verzeichnis gefunden bzw. geöffnet werden?
    if(dir == NULL) {
        cerr << "Dieses Verzeichnis kann nicht geöffnet werden!" << endl;
        return false;
    }
    while((entry = readdir(dir))!=nullptr) {
        string path = sp + "/" + entry->d_name;
        struct stat fileStat; // stat = POSIX Funktion, liefert Infos über eine Datei/ein Verzeichnis; struct speichert mehrere Eigenschaften auf einmal
        // Checkt, ob Infos gelesen wurden = 0; wenn Fehler z.B. keine Berechtigung = -1
        if(stat(path.c_str(), &fileStat) < 0) { // fileStat -> detaillierte Informationen über die Datei/das Verzeichnis (z.B. erkennen, ob es ein Verzeichnis oder eine Datei ist)
            cout << entry->d_name << " konnte nicht gelesen werden." << endl;
            continue;
        }

        // Checken, ob Datei ist
        if(S_ISREG(fileStat.st_mode)) {
            string currentFileName = entry->d_name;
            // Checken, ob casesensitive true ist
            string targetFileName = filename;
            if(!casesensitive) {
                currentFileName = lowerCase(currentFileName);
                targetFileName = lowerCase(targetFileName);
            }
            if(currentFileName == targetFileName) {
                char absolutePath[PATH_MAX];
                if(realpath(path.c_str(), absolutePath) != nullptr) {
                    cout << getpid() << ": " << currentFileName << ": " << absolutePath << endl;
                    fileFound = true;
                }
            }
        }else if(recursive && S_ISDIR(fileStat.st_mode) && strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
            if(search(path, filename, recursive, casesensitive)) {
                fileFound = true;
            }
        }
    }

    closedir(dir);
    return fileFound;
}

int main(int argc, char* argv[]) {
    bool recursive = false;
    bool casesensitive = true;
    const char *searchpath = NULL;

    int opt;
    while ((opt = getopt(argc, argv, "Ri")) != -1) {
        switch(opt) {
            case 'R':
                recursive = true;
                break;
            case 'i':
                casesensitive = false;
                break;
            default:
                cerr << "Unbekannte Option: " << char(optopt) << endl;
                return 1;
        }
    }

    if (argc < 2) {
        std::cerr << "Pfad und Dateinamen angeben!\n";
        return 1;
    }

    fs::path p(argv[optind]);
    if (!fs::exists(p) || !fs::is_directory(p)) {
        cerr << "Kein gültiger Pfad!" << endl;
        return 1;
    }

    // save searchpath & filenames
    searchpath = argv[optind];
    vector<string> files;
    for (int i = optind+1; i < argc; i++) {
        files.push_back(argv[i]);
    }

    for(string file : files) {
        pid_t pid = fork();
        if(pid < 0) {
            // Fork failed
            cerr << "Fork failed" << endl;
            return 1;
        }else if(pid == 0) {
            // Start child process
            bool fileFound = search(searchpath, file, recursive, casesensitive);
            if(!fileFound) {
                cerr << "File " << file << " not found!" << endl;
                return 1;
            }
        }
    }

    // Parent wartet auf childs:
    int status;
    bool anyFileNotFound = false;
    for(size_t i = 0; i < files.size(); i++) {
        wait(&status);
        if(WIFEXITED(status) && WEXITSTATUS(status) != EXIT_SUCCESS) {
            anyFileNotFound = true;
        }
    }

    if(anyFileNotFound) {
        return 1;
    }

    return 0;
}
