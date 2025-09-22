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

string lowerCase(const string &s) {
    string out = s;
    for(int i=0; i < out.size(); i++) {
        // zu unsigned char casten = positiver Wert, weil signed char oft negativ
        // weil an tolower() muss ein positiver Wert übergeben wird
        out[i] = tolower(static_cast<unsigned char>(out[i]));
    }
    return out;
}

bool search(const string &sp, const string &filename, bool recursive, bool casesensitive) {
    // umwandeln in const char*, weil viele Fkt. es so erwarten
    const char* searchpath = sp.c_str();
    // Öffnet das Verzeichnis; bei Fehler nullptr
    DIR *dir = opendir(searchpath);
    // kann das Verzeichnis gefunden bzw. geöffnet werden?
    if(dir == NULL) {
        cerr << "Verzeichnis " << sp << " kann nicht geöffnet werden!" << strerror(errno) << endl;
        return false;
    }
    // dirent ... vordefinierte Struktur aus <dirent.h>, die generelle Informationen über einen Eintrag in einem Verzeichnis speichert (z. B. Dateinamen)
    struct dirent *entry;
    bool fileFound = false;
    // readdir liest den nächsten Eintrag aus dem geöffneten Verzeichnis dir
    // liest, solange es Einträge gibt, sonst nullptr
    while((entry = readdir(dir))!=nullptr) {
        // . und .. sind das aktuelle Verzeichnis --> weglassen, sonst Endlosschleife (strcmp vergleicht String; 0 bedeutet sind gleich)
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) continue;
        string path = sp + "/" + entry->d_name;
        // struct speichert mehrere Eigenschaften auf einmal
        // stat = POSIX Funktion, liefert detaillierte Infos über eine Datei/ein Verzeichnis
        struct stat fileStat;
        // Checkt, ob Infos gelesen wurden = 0; wenn Fehler z.B. keine Berechtigung = -1
        if(stat(path.c_str(), &fileStat) < 0) {
            cout << entry->d_name << " konnte nicht gelesen werden: " << endl;
            continue;
        }

        // mit S_ISREG checken, ob Datei ist
        if(S_ISREG(fileStat.st_mode)) {
            string currentFileName = entry->d_name;
            string targetFileName = filename;
            // Checken, ob casesensitive true ist
            if(!casesensitive) {
                // Falls Groß-/Kleinschreibung egal ist, sowohl gesuchtes als auch aktuelles File auf lowercase bringen
                currentFileName = lowerCase(currentFileName);
                targetFileName = lowerCase(targetFileName);
            }
            // Falls das gesuchte File denselben Namen hat wie das aktuelle
            if(currentFileName == targetFileName) {
                // Array von Zeichen mit der Größe PATH_MAX (längst möglicher Pfad)
                char absolutePath[PATH_MAX];
                // realpath wandelt einen relativen Pfad (z.B. ./ordner/file.txt) in einen Absoluten um (/home/user/projekt/ordner/file.txt))
                // nullptr --> Datei existiert nicht oder keine Berechtigung
                if(realpath(path.c_str(), absolutePath) != nullptr) {
                    fileFound = true;
                    // ohne cout, weil mehrere Kindprozesse gleichzeitig schreiben könnten und irgendein Blödsinn raus kommt; lieber write() verwenden weil es ohne Puffer arbeitet
                    string line = to_string(getpid()) + ": " + filename + ": " + absolutePath + "\n";
                    // Standardausgabe mit STDOUT_FILENO
                    write(STDOUT_FILENO, line.c_str(), line.size());
                }
            }
        // wenn es keine Datei ist (und somit ein Directory)
        // wird nur aufgerufen, wenn recursiv, sonst würde er ja die Ordner nicht durchgehen
        }else if(recursive && S_ISDIR(fileStat.st_mode)) {
            // ruft recursiv nochmals die Methode auf, diesmal ist aber in path nun der Unterordner eingetragen
            if(search(path, filename, recursive, casesensitive)) {
                fileFound = true;
            }
        }
    }

    // Verzeichnis schließen und Ressourcen freigeben
    closedir(dir);
    return fileFound;
}

int main(int argc, char* argv[]) {
    bool recursive = false;
    bool casesensitive = true;

    int opt;
    // getopt liefert bei jeden Aufruf den nächsten Optionsbuchstaben(bei uns i und R)
    // argc = Anzahl der Argumente
    // argv = liefert Argument an einem bestimmten Index z.B. argv[i]
    while((opt = getopt(argc, argv, "Ri")) != -1) {
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

    // optind = erster index, der keine Option (-R, -i) ist, sondern ein anderes Argument (z.B. Pfad oder die Filenames)
    // checkt ob optind >= argc --> wenn ja, wurden kein Pfad und Filename eingegeben
    if (optind >= argc) {
        cerr << "Usage: " << argv[0] << " [-R] [-i] searchpath filename1 [filename2 ...]" << endl;
        return 1;
    }

    // erste nicht Option ist dann der Pfad
    string searchpath = argv[optind];
    // aus dem namespace <filesystem>; erleichtert Umgang mit Pfaden
    fs::path p(searchpath);
    // checken, ob der vom User angegebene Pfad überhaupt existiert und ob es eh ein Pfad ist
    if(!fs::exists(p) || !fs::is_directory(p)) {
        cerr << "Kein gültiger Pfad: " << searchpath << endl;
        return 1;
    }

    // Alle filenames, die der User eingegeben hat, in einem vector speichern
    vector<string> files;
    for(int i = optind+1; i < argc; i++) {
        files.push_back(argv[i]);
    }

    // Vector mit allen Kindern
    vector<pid_t> child_pids;
    // For schleife über alle Files (für jedes FIle soll ein Kindprozess gestartet werden)
    for (const string &file : files) {
        // Dupliziert sozusagen den aktuellen Prozess imd ,acht daraus ein Child
        pid_t pid = fork();
        // bei pid < 0 ... Fehler
        if(pid < 0) {
            cerr << "Fork fehlgeschlagen für '" << file << "': " << strerror(errno) << endl;
            break;
        // bei pid == 0 -> Child-Prozess: führt Suche durch und beendet sich danach
        } else if (pid == 0) {
            bool fileFound = search(searchpath, file, recursive, casesensitive);
            if(!fileFound) {
                // Error Ausgabe
                string errline = to_string(getpid()) + ": " + file + ": not found\n";
                write(STDERR_FILENO, errline.c_str(), errline.size());
                // Child beendet mit Fehlercode
                _exit(1);
            } else {
                // Child beendet erfolgreich
                _exit(0);
            }
        } else {
            // weder <0 noch 0 -> daher Parent: merke Kinder-PID
            child_pids.push_back(pid);
        }
    }

    // Parent wartet auf alle Kinder (vermeidet Zombies)
    int status = 0;
    bool anyChildFailed = false;

    // wissen, wie viele Kinder wir haben -> feste Schleife mit waitpid() für jede PID
    for (pid_t pid : child_pids) {
        // waitpid sollte die pid zurückgeben - bei Fehler aber -1
        if(waitpid(pid, &status, 0) == -1) {
            // Fehler beim Warten (z. B. PID existiert nicht mehr)
            cerr << "Fehler bei waitpid für Kind " << pid << ": " << strerror(errno) << endl;
            anyChildFailed = true;
            continue;
        }
        // WIFEXITED true, wenn Kind normal beendet wurde
        if(WIFEXITED(status)) {
            int es = WEXITSTATUS(status);
            // wenn der Exitstatus nicht 0 ist, ist das Kind fehlerhaft beendet
            if(es != 0) {
                anyChildFailed = true;
            }
        } else {
            // WIFEXITED false, wenn Kind fehlerhaft beendet wurde
            anyChildFailed = true;
        }
    }

    if(anyChildFailed) {
        return 1;
    }

    return 0;
}
