SD2 — Music Player

The program supports:

- Reading song information from metadata using TagLib.
- Deleting songs and removing albums when they become empty.
- Moving individual songs within the playlist and album tracks within their album.
- Importing individual MP3/FLAC songs and albums from folders.
- Sorting playlist elements by artist and album tracks by track number using quicksort.
- Sequential, random, and shuffle playback. Shuffle keeps each album’s songs together.
- Saving and loading playlists in JSON format.

The outer playlist uses a separate `CircularDLL` class built around `std::list`, with circular navigation. Each playlist element contains a regular `std::list<Song>`. Playback uses `std::queue`, which stores references to songs through their addresses. Sorting rearranges existing list nodes using `splice`.

To build and run:

1. Download or clone the repository.
2. Open `MuzikosGrotuvas.sln` in Visual Studio.
3. Select Release and x64.

C++17, the Desktop development with C++ workload, and C++ CMake tools for Windows are required. The project selects the v145 platform toolset. The dependency setup script uses Visual Studio 2022 and v143. For a Visual Studio 2022 build, select v143 under the project’s Configuration Properties → General → Platform Toolset.

Before compilation, `setup_dependencies.ps1` automatically downloads and builds TagLib and zlib and downloads the nlohmann/json header. Internet access is required during the initial setup.

For a quick test, enter the following in the program, pressing Enter after each line:

14
test_music/test.json
1

This loads the supplied playlist and displays its songs. To sort the fourth element’s album tracks, enter `9`, followed by `4`. Commands `10`, `11`, and `12` start sequential, random, and shuffle playback respectively. Each asks how many songs to display. Enter `0` to exit.

Song and album operations use list positions starting from 1. A song’s position can differ from its metadata track number.

JSON files store group types and song paths. Paths inside JSON are relative to that JSON file’s folder, and song metadata is read again during loading. Keep the supplied folder structure and referenced audio files together. Paths entered through the menu are relative to the program’s working directory, or absolute paths can be entered.

Loading a valid JSON playlist replaces the current playlist. If loading fails, the current playlist is retained.