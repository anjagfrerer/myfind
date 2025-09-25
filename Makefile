# Compiler und Flags
CXX = g++
# -std=c++17 (aktiviert Features von C++17, z. B.: <filesystem>)
# -Wall (Aktiviert die häufigsten Warnungen des Compilers)
# -Wextra (Aktiviert zusätzliche Warnungen)
# -pedantic (Erzwingt strikte Einhaltung des C++-Standards)
# Optimierungen beim Kompilieren
CXXFLAGS = -std=c++17 -Wall -Wextra -pedantic -O2

# Name des Executables
TARGET = myfind

# Quellcode-Dateien
SRC = myfind.cpp

# Standardziel: build
all: $(TARGET)

# Regel zum Kompilieren
$(TARGET): $(SRC)
	g++ $(CXXFLAGS) -o $(TARGET) $(SRC)

# "clean"-Ziel zum Löschen von Executable und temporären Dateien
clean:
	rm -f $(TARGET)

# Optional: phony targets
.PHONY: all clean
