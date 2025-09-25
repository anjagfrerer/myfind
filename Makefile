# Compiler und Flags
CXX = g++
# -std=c++17 (aktiviert Features von C++17, z. B.: <filesystem>)
# -Wall (Aktiviert die häufigsten Warnungen des Compilers)
# -Wextra (Aktiviert zusätzliche Warnungen)
# -pedantic (Erzwingt strikte Einhaltung des C++-Standards)
# Optimierungen beim Kompilieren
CXXFLAGS = -std=c++17 -Wall -Wextra -pedantic -O2

#Name des executables (des fertigen Programmes)
TARGET = myfind

#SRC = Variable für die Quellcode-Dateien
SRC = myfind.cpp

#Standardziel (bei "make" wird dieses Ziel ausgeführt)
all: $(TARGET)

#Regel zum Kompilieren
# bei uns: g++ -std=c++17 -Wall -Wextra -pedantic -O2 -o myfind myfind.cpp
$(TARGET): $(SRC) #SRC ... Abhängigkeiten
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SRC)

#zum Löschen von Executable und temporären Dateien
# rm ... löschen ; -f ... force (keine Fehlermeldung, wenn die Datei nicht existiert)
clean:
	rm -f $(TARGET)