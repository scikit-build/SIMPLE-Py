from importlib.resources import files

def show_file():
    cmake_file = files("example") / "file.txt" 
    print(cmake_file.read_text())
