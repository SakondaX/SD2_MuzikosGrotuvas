#include <iostream>
#include <list>
#include <queue>
#include <string>
#include <limits>
#include <utility>
#include <random>
#include <filesystem>
#include <fstream>
#include <chrono>
#include <stdexcept>
#include <system_error>
#include <nlohmann/json.hpp>
#include <fileref.h>
#include <tfilestream.h>
#include <tag.h>
#include <audioproperties.h>
#include <mpegfile.h>
#include <mpegproperties.h>
#include <flacfile.h>
#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#endif
using namespace std;
using nlohmann::json;
//Dainos ir grojarascio elemento duomenys
struct Song
{
    string path;
    string artist;
    string album;
    int trackNumber=0; //0 - numeris albume nenurodytas.
    string title;
};
enum class PlaylistElementType
{
    Song,
    Album
};
struct PlaylistElement
{
    PlaylistElementType type=PlaylistElementType::Song;
    list<Song> songs;
};
//Saraso mazgai ir quicksort
template <typename List>
auto findListNode(List &elements,size_t pos)
{
    size_t count=elements.size();
    if(pos<1 || pos>count)
    {
        return elements.end();
    }
    auto cur=elements.begin();
    if(pos-1<=count-pos)
    {
        for(size_t i=1;i<pos;i++)
        {
            ++cur;
        }
    }
    else
    {
        cur=elements.end();
        --cur;
        for(size_t i=count;i>pos;i--)
        {
            --cur;
        }
    }
    return cur;
}
template <typename T>
typename list<T>::iterator firstAfter(list<T> &elements,typename list<T>::iterator before)
{
    if(before==elements.end())
    {
        return elements.begin();
    }
    ++before;
    return before;
}
template <typename T,typename Compare>
void sortRange(list<T> &elements,typename list<T>::iterator before,
    typename list<T>::iterator after,size_t count,Compare &less)
{
    while(count>1)
    {
        auto first=firstAfter(elements,before);
        auto middle=first;
        for(size_t i=0;i<count/2;i++)
        {
            ++middle;
        }
        const T &pivot=*middle;
        auto lessTail=before;
        auto equalTail=before;
        auto cur=first;
        size_t lessCount=0;
        size_t equalCount=0;
        for(size_t i=0;i<count;i++)
        {
            auto node=cur;
            ++cur;
            if(less(*node,pivot))
            {
                elements.splice(firstAfter(elements,lessTail),elements,node);
                lessTail=node;
                lessCount++;
                if(equalCount==0)
                {
                    equalTail=node;
                }
            }
            else if(!less(pivot,*node))
            {
                elements.splice(firstAfter(elements,equalTail),elements,node);
                equalTail=node;
                equalCount++;
            }
        }
        auto firstEqual=firstAfter(elements,lessTail);
        size_t greaterCount=count-lessCount-equalCount;
        //Rekursija tik mazesnei pusei, kita naudoja cikla.
        if(lessCount<greaterCount)
        {
            sortRange(elements,before,firstEqual,lessCount,less);
            before=equalTail;
            count=greaterCount;
        }
        else
        {
            sortRange(elements,equalTail,after,greaterCount,less);
            after=firstEqual;
            count=lessCount;
        }
    }
}
template <typename T,typename Compare>
void quickSortList(list<T> &elements,Compare less)
{
    sortRange(elements,elements.end(),elements.end(),elements.size(),less);
}
//Ciklinis dvikryptis sarasas
template <typename T>
class CircularDLL
{
private:
    list<T> elements;
    typename list<T>::iterator current;
public:
    CircularDLL()
    {
        current=elements.end();
    }
    //Nukopijuotas iteratorius rodytu i seno saraso elementa.
    CircularDLL(const CircularDLL&)=delete;
    CircularDLL& operator=(const CircularDLL&)=delete;
    ~CircularDLL()=default;
    bool isEmpty() const
    {
        return elements.empty();
    }
    size_t size() const
    {
        return elements.size();
    }
    T &addNodeEnd()
    {
        elements.emplace_back();
        if(elements.size()==1)
        {
            current=elements.begin();
        }
        return elements.back();
    }
    T &addNodeEnd(T &&value)
    {
        elements.push_back(move(value));
        if(elements.size()==1)
        {
            current=elements.begin();
        }
        return elements.back();
    }
    const T *getCurrent() const
    {
        if(elements.empty())
        {
            return nullptr;
        }
        return &*current;
    }
    T *getNodePosition(size_t pos)
    {
        auto cur=findListNode(elements,pos);
        if(cur==elements.end())
        {
            return nullptr;
        }
        return &*cur;
    }
    bool deleteNodePosition(size_t pos)
    {
        auto cur=findListNode(elements,pos);
        if(cur==elements.end())
        {
            return false;
        }
        if(cur==current)
        {
            current=elements.erase(cur);
            if(current==elements.end() && !elements.empty())
            {
                current=elements.begin();
            }
        }
        else
        {
            elements.erase(cur);
        }
        return true;
    }
    bool moveNodePosition(size_t pos,size_t newPos)
    {
        if(pos<1 || pos>elements.size() || newPos<1 || newPos>elements.size())
        {
            return false;
        }
        if(pos==newPos)
        {
            return true;
        }
        auto cur=findListNode(elements,pos);
        auto target=findListNode(elements,newPos);
        if(pos<newPos)
        {
            ++target;
        }
        elements.splice(target,elements,cur);
        return true;
    }
    template <typename Compare>
    void quickSort(Compare less)
    {
        quickSortList(elements,less);
    }
    void next()
    {
        if(elements.empty())
        {
            return;
        }
        ++current;
        if(current==elements.end())
        {
            current=elements.begin();
        }
    }
    void previous()
    {
        if(elements.empty())
        {
            return;
        }
        if(current==elements.begin())
        {
            current=elements.end();
        }
        --current;
    }
    void clear()
    {
        elements.clear();
        current=elements.end();
    }
    void replaceWith(CircularDLL &other)
    {
        if(this==&other)
        {
            return;
        }
        clear();
        elements.splice(elements.end(),other.elements);
        current=elements.begin();
        other.current=other.elements.end();
    }
    const list<T> &getElements() const
    {
        return elements;
    }
};
//Failu keliai ir metaduomenys
bool isSpace(char symbol)
{
    return symbol==' ' || symbol=='\t' || symbol=='\r' ||
        symbol=='\n' || symbol=='\v' || symbol=='\f';
}
bool isBlank(const string &text)
{
    for(char symbol : text)
    {
        if(!isSpace(symbol))
        {
            return false;
        }
    }
    return true;
}
string trim(const string &text)
{
    size_t first=0;
    size_t last=text.size();
    while(first<last && isSpace(text[first]))
    {
        first++;
    }
    while(last>first && isSpace(text[last-1]))
    {
        last--;
    }
    return text.substr(first,last-first);
}
bool parsePath(const string &inputPath,filesystem::path &path,string &error)
{
    if(inputPath.find('\0')!=string::npos)
    {
        error="Kelias negali tureti NUL simbolio.";
        return false;
    }
    string pathText=trim(inputPath);
    if(pathText.empty())
    {
        error="Iveskite kelia.";
        return false;
    }
    if(pathText.front()=='"' || pathText.back()=='"')
    {
        if(pathText.size()<2 || pathText.front()!='"' || pathText.back()!='"')
        {
            error="Kelio dvigubos kabutes turi buti porines.";
            return false;
        }
        pathText=pathText.substr(1,pathText.size()-2);
    }
    if(pathText.empty())
    {
        error="Iveskite kelia.";
        return false;
    }
    if(pathText.find('"')!=string::npos)
    {
        error="Dvigubos kabutes leidziamos tik kelio isoreje.";
        return false;
    }
    path=filesystem::u8path(pathText);
    error.clear();
    return true;
}
string getExtension(const filesystem::path &path)
{
    string extension=path.extension().u8string();
    for(char &symbol : extension)
    {
        if(symbol>='A' && symbol<='Z')
        {
            symbol=static_cast<char>(symbol-'A'+'a');
        }
    }
    return extension;
}
bool readSong(const string &inputPath,Song &song,string &error)
{
    try
    {
        filesystem::path path;
        if(!parsePath(inputPath,path,error))
        {
            return false;
        }
        string extension=getExtension(path);
        if(extension!=".mp3" && extension!=".flac")
        {
            error="Leidziami tik MP3 ir FLAC failai.";
            return false;
        }
        error_code pathError;
        filesystem::file_status status=filesystem::status(path,pathError);
        if(status.type()==filesystem::file_type::not_found)
        {
            error="Nurodytas failas neegzistuoja.";
            return false;
        }
        if(pathError)
        {
            error="Nepavyko patikrinti failo. Patikrinkite kelia ir prieigos teises.";
            return false;
        }
        if(!filesystem::is_regular_file(status))
        {
            error="Nurodykite paprasta faila, o ne aplanka ar kita objekta.";
            return false;
        }
        path=filesystem::canonical(path,pathError);
        if(pathError)
        {
            error="Nepavyko nustatyti absoliutaus failo kelio.";
            return false;
        }
        //true - failas skaitomas
        TagLib::FileStream stream(path.c_str(),true);
        TagLib::FileRef file(&stream,true);
        if(file.isNull())
        {
            error="Nepavyko perskaityti failo. Jis gali buti sugadintas arba neprieinamas.";
            return false;
        }
        const TagLib::AudioProperties *audio=file.audioProperties();
        if(audio==nullptr || audio->sampleRate()<=0 || audio->channels()<=0)
        {
            error="Faile nerastas tinkamas garso irasas.";
            return false;
        }
        if(extension==".mp3")
        {
            const TagLib::MPEG::File *mp3=dynamic_cast<const TagLib::MPEG::File*>(file.file());
            if(mp3==nullptr || mp3->audioProperties()==nullptr || mp3->audioProperties()->layer()!=3)
            {
                error="Failo turinys neatitinka MP3 formato.";
                return false;
            }
        }
        else if(dynamic_cast<const TagLib::FLAC::File*>(file.file())==nullptr)
        {
            error="Failo turinys neatitinka FLAC formato.";
            return false;
        }
        const TagLib::Tag *tag=file.tag();
        if(tag==nullptr)
        {
            error="Nepavyko perskaityti failo metaduomenu.";
            return false;
        }
        unsigned int track=tag->track();
        if(track>static_cast<unsigned int>(numeric_limits<int>::max()))
        {
            error="Dainos numeris albume per didelis.";
            return false;
        }
        Song parsed;
        parsed.path=path.u8string();
        parsed.artist=tag->artist().to8Bit(true);
        parsed.album=tag->album().to8Bit(true);
        parsed.trackNumber=static_cast<int>(track);
        parsed.title=tag->title().to8Bit(true);
        song=move(parsed);
        error.clear();
        return true;
    }
    catch(const exception &problem)
    {
        error="Nepavyko perskaityti dainos arba jos kelio. ";
        error+=problem.what();
        return false;
    }
}
bool readAlbum(const string &inputPath,PlaylistElement &album,string &error)
{
    try
    {
        filesystem::path path;
        if(!parsePath(inputPath,path,error))
        {
            return false;
        }
        error_code pathError;
        filesystem::file_status status=filesystem::status(path,pathError);
        if(status.type()==filesystem::file_type::not_found)
        {
            error="Nurodytas aplankas neegzistuoja.";
            return false;
        }
        if(pathError)
        {
            error="Nepavyko patikrinti aplanko. Patikrinkite kelia ir prieigos teises.";
            return false;
        }
        if(!filesystem::is_directory(status))
        {
            error="Nurodykite albumo aplanka, o ne faila.";
            return false;
        }
        path=filesystem::canonical(path,pathError);
        if(pathError)
        {
            error="Nepavyko nustatyti absoliutaus aplanko kelio.";
            return false;
        }
        filesystem::directory_iterator current(path,pathError);
        if(pathError)
        {
            error="Nepavyko atverti aplanko: "+path.u8string();
            return false;
        }
        PlaylistElement parsed;
        parsed.type=PlaylistElementType::Album;
        while(current!=filesystem::directory_iterator())
        {
            const filesystem::directory_entry &entry=*current;
            string extension=getExtension(entry.path());
            if(extension==".mp3" || extension==".flac")
            {
                filesystem::file_status entryStatus=entry.status(pathError);
                if(pathError)
                {
                    error="Nepavyko patikrinti failo: "+entry.path().u8string();
                    return false;
                }
                if(filesystem::is_regular_file(entryStatus))
                {
                    Song song;
                    string songError;
                    if(!readSong(entry.path().u8string(),song,songError))
                    {
                        error="Failas: "+entry.path().u8string()+". "+songError;
                        return false;
                    }
                if(isBlank(song.artist) || isBlank(song.album))
                    {
                        error="Failas: "+song.path+". Albumo importui reikia atlikejo ir albumo zymu.";
                        return false;
                    }
                    if(!parsed.songs.empty())
                    {
                        const Song &first=parsed.songs.front();
                        if(song.artist!=first.artist || song.album!=first.album)
                        {
                            error="Misrus aplankas. Failas: "+song.path+". Atlikejas: "+song.artist+
                                "; albumas: "+song.album+". Tiketasi atlikejo: "+first.artist+
                                "; albumo: "+first.album+". Visas albumo importas nutrauktas.";
                            return false;
                        }
                    }
                    parsed.songs.push_back(move(song));
                }
            }
            current.increment(pathError);
            if(pathError)
            {
                error="Nepavyko perskaityti visu aplanko failu: "+path.u8string();
                return false;
            }
        }
        if(parsed.songs.empty())
        {
            error="Aplanke nerasta MP3 arba FLAC dainu. Poaplankiai netikrinami.";
            return false;
        }
        album=move(parsed);
        error.clear();
        return true;
    }
    catch(const exception &problem)
    {
        error="Nepavyko perskaityti albumo arba jo kelio. ";
        error+=problem.what();
        return false;
    }
}
//JSON save & load
bool getJsonPath(const string &inputPath,filesystem::path &path,string &error)
{
    if(!parsePath(inputPath,path,error))
    {
        return false;
    }
    if(getExtension(path)!=".json")
    {
        error="Nurodykite faila su .json pletiniu.";
        return false;
    }
    error_code pathError;
    path=filesystem::absolute(path,pathError);
    if(pathError)
    {
        error="Nepavyko nustatyti JSON failo kelio.";
        return false;
    }
    filesystem::path parent=filesystem::canonical(path.parent_path(),pathError);
    if(pathError || !filesystem::is_directory(parent))
    {
        error="JSON failo aplankas turi egzistuoti ir buti pasiekiamas.";
        return false;
    }
    path=parent/path.filename();
    return true;
}
bool validateElement(const PlaylistElement &element,string &error)
{
    if(element.type!=PlaylistElementType::Song && element.type!=PlaylistElementType::Album)
    {
        error="Nezinomas grojarascio elemento tipas.";
        return false;
    }
    if(element.songs.empty() || (element.type==PlaylistElementType::Song && element.songs.size()!=1))
    {
        error="Paviene daina turi tureti viena kelia, albumas - bent viena.";
        return false;
    }
    if(element.type==PlaylistElementType::Album)
    {
        const Song &first=element.songs.front();
        if(isBlank(first.artist) || isBlank(first.album))
        {
            error="Albumui reikia atlikejo ir albumo zymu.";
            return false;
        }
        for(const Song &song : element.songs)
        {
            if(song.artist!=first.artist || song.album!=first.album)
            {
                error="Vieno albumo dainu atlikejo ir albumo zymos turi sutapti. Failas: "+song.path;
                return false;
            }
        }
    }
    return true;
}
bool getRelativePath(const Song &song,const filesystem::path &parent,string &relative,string &error)
{
    if(song.path.empty() || song.path.find('\0')!=string::npos)
    {
        error="Dainos kelias negali buti tuscias arba tureti NUL simbolio.";
        return false;
    }
    filesystem::path source=filesystem::u8path(song.path);
    string extension=getExtension(source);
    if(!source.is_absolute() || (extension!=".mp3" && extension!=".flac"))
    {
        error="Dainos kelias turi buti absoliutus MP3 arba FLAC kelias: "+song.path;
        return false;
    }
    error_code pathError;
    filesystem::path result=filesystem::relative(source,parent,pathError);
    if(pathError || result.empty() || result.has_root_name() || result.has_root_directory())
    {
        error="Nepavyko sudaryti santykinio dainos kelio. JSON ir dainas laikykite tame paciame diske: "+song.path;
        return false;
    }
    relative=result.generic_u8string();
    return true;
}
class TemporaryDocument
{
private:
    filesystem::path directory;
    filesystem::path file;
public:
    TemporaryDocument()=default;
    TemporaryDocument(const TemporaryDocument&)=delete;
    TemporaryDocument& operator=(const TemporaryDocument&)=delete;
    ~TemporaryDocument()
    {
        error_code pathError;
        if(!file.empty())
        {
            filesystem::remove(file,pathError);
        }
        if(!directory.empty())
        {
            filesystem::remove(directory,pathError);
        }
    }
    bool create(const filesystem::path &parent,string &error)
    {
        string name=".sd2-save-"+to_string(chrono::steady_clock::now().time_since_epoch().count());
        for(int i=0;i<128;i++)
        {
            filesystem::path candidate=parent/(name+"-"+to_string(i));
            error_code pathError;
            if(filesystem::create_directory(candidate,pathError))
            {
                directory=move(candidate);
                file=directory/"playlist.json";
                return true;
            }
            if(pathError && pathError!=errc::file_exists)
            {
                error="Nepavyko sukurti laikino JSON dokumento. Patikrinkite aplanko prieigos teises.";
                return false;
            }
        }
        error="Nepavyko parinkti laikino JSON dokumento vardo.";
        return false;
    }
    const filesystem::path &getPath() const
    {
        return file;
    }
};
bool savePlaylist(const CircularDLL<PlaylistElement> &playlist,const string &inputPath,string &error)
{
    try
    {
        filesystem::path path;
        if(!getJsonPath(inputPath,path,error))
        {
            return false;
        }
        error_code pathError;
        filesystem::file_status status=filesystem::symlink_status(path,pathError);
        if((pathError && status.type()!=filesystem::file_type::not_found) ||
            (status.type()!=filesystem::file_type::not_found && !filesystem::is_regular_file(status)))
        {
            error="JSON galima issaugoti tik i paprasta faila, ne aplanka arba nuoroda.";
            return false;
        }
        //Patikrina modeli ir kelius pries rasant
        for(const PlaylistElement &element : playlist.getElements())
        {
            if(!validateElement(element,error))
            {
                return false;
            }
        }
        TemporaryDocument temporary;
        if(!temporary.create(path.parent_path(),error))
        {
            return false;
        }
        ofstream output(temporary.getPath(),ios::binary|ios::trunc);
        if(!output)
        {
            error="Nepavyko atverti laikino JSON failo rasymui.";
            return false;
        }
        output << "{" << endl;
        output << "  \"version\": 1," << endl;
        output << "  \"elements\": [" << endl;
        size_t elementPosition=0;
        for(const PlaylistElement &element : playlist.getElements())
        {
            output << "    {" << endl;
            output << "      \"type\": \"" << (element.type==PlaylistElementType::Song ? "song" : "album") << "\"," << endl;
            output << "      \"songs\": [" << endl;
            size_t songPosition=0;
            for(const Song &song : element.songs)
            {
                string relative;
                if(!getRelativePath(song,path.parent_path(),relative,error))
                {
                    return false;
                }
                //Biblioteka tinkamai uzkoduoja kabutes ir kitus JSON simbolius
                output << "        " << json(relative).dump();
                if(++songPosition<element.songs.size())
                {
                    output << ",";
                }
                output << endl;
            }
            output << "      ]" << endl;
            output << "    }";
            if(++elementPosition<playlist.size())
            {
                output << ",";
            }
            output << endl;
        }
        output << "  ]" << endl;
        output << "}" << endl;
        //close() iraso likusius duomenis; po jo patikrinamos rasymo klaidas.
        output.close();
        if(!output)
        {
            error="Nepavyko uzbaigti JSON failo irasymo. Ankstesnis failas nepakeistas.";
            return false;
        }
        //Senas failas keiciamas tik kai visas naujas dokumentas jau irasytas.
        filesystem::rename(temporary.getPath(),path,pathError);
        if(pathError)
        {
            error="Nepavyko pakeisti JSON failo. Ankstesnis failas nepakeistas; patikrinkite prieigos teises.";
            return false;
        }
        error.clear();
        return true;
    }
    catch(const exception &problem)
    {
        error="Nepavyko issaugoti JSON dokumento arba apdoroti failo kelio. ";
        error+=problem.what();
        return false;
    }
}
bool loadPlaylist(CircularDLL<PlaylistElement> &playlist,const string &inputPath,string &error)
{
    try
    {
        filesystem::path path;
        if(!getJsonPath(inputPath,path,error))
        if(!getJsonPath(inputPath,path,error))
        {
            return false;
        }
        error_code pathError;
        filesystem::file_status status=filesystem::status(path,pathError);
        if(pathError || !filesystem::is_regular_file(status))
        {
            error="JSON failas neegzistuoja arba yra nepasiekiamas.";
            return false;
        }
        path=filesystem::canonical(path,pathError);
        if(pathError)
        {
            error="Nepavyko nustatyti JSON failo vietos.";
            return false;
        }
        ifstream input(path,ios::binary);
        if(!input)
        {
            error="Nepavyko atverti JSON failo skaitymui.";
            return false;
        }
        string text;
        char symbol;
        while(input.get(symbol))
        {
            text+=symbol;
        }
        if(input.bad() || !input.eof())
        {
            error="Nepavyko perskaityti viso JSON failo.";
            return false;
        }
        if(text.find('\0')!=string::npos)
        {
            error="JSON dokumentas negali tureti neuzkoduoto NUL simbolio.";
            return false;
        }
        json document=json::parse(text);
        if(!document.is_object() || document.size()!=2 || !document.contains("version") ||
            !document.contains("elements") || !document["version"].is_number_integer() ||
            document["version"]!=1 || !document["elements"].is_array())
        {
            error="Netinkama JSON struktura. Reikia sveikojo version: 1 ir elements masyvo.";
            return false;
        }
        //Naujas grojarastis kuriamas atskirai. Senas dar naudojamas nepakeistas.
        CircularDLL<PlaylistElement> parsed;
        size_t elementPosition=0;
        for(const json &entry : document["elements"])
        {
            ++elementPosition;
            if(!entry.is_object() || entry.size()!=2 || !entry.contains("type") || !entry.contains("songs") ||
                !entry["type"].is_string() || !entry["songs"].is_array())
            {
                error="JSON elementas "+to_string(elementPosition)+": reikia type ir songs lauku.";
                return false;
            }
            string type=entry["type"];
            if((type!="song" && type!="album") || entry["songs"].empty() ||
                (type=="song" && entry["songs"].size()!=1))
            {
                error="JSON elementas "+to_string(elementPosition)+": netinkamas tipas arba dainu kiekis.";
                return false;
            }
            PlaylistElement element;
            element.type=type=="song" ? PlaylistElementType::Song : PlaylistElementType::Album;
            for(const json &value : entry["songs"])
            {
                if(!value.is_string())
                {
                    error="JSON dainos kelias turi buti tekstas.";
                    return false;
                }
                string relative=value;
                if(relative.empty() || relative.find('\0')!=string::npos)
                {
                    error="JSON dainos kelias negali buti tuscias arba tureti NUL simbolio.";
                    return false;
                }
                filesystem::path songPath=filesystem::u8path(relative);
                if(songPath.has_root_name() || songPath.has_root_directory())
                {
                    error="JSON saugomi tik santykiniai dainu keliai: "+relative;
                    return false;
                }
                songPath=path.parent_path()/songPath;
                Song song;
                if(!readSong(songPath.u8string(),song,error))
                {
                    error="JSON elementas "+to_string(elementPosition)+", failas "+relative+". "+error;
                    return false;
                }
                element.songs.push_back(move(song));
            }
            if(!validateElement(element,error))
            {
                error="JSON elementas "+to_string(elementPosition)+". "+error;
                return false;
            }
            parsed.addNodeEnd(move(element));
        }
        //Perkeliami jau patikrinti saraso mazgai, nekopijuojant senu dainu.
        playlist.replaceWith(parsed);
        error.clear();
        return true;
    }
    catch(const exception &problem)
    {
        error="Nepavyko perskaityti JSON dokumento arba jo failu. ";
        error+=problem.what();
        return false;
    }
}
//Grojimo eile ir modes
enum class PlaybackMode
{
    Sequential,
    Random,
    Shuffle
};
struct QueueEntry
{
    const PlaylistElement *element;
    const Song *song;
};
void addGroup(queue<QueueEntry> &playQueue,const PlaylistElement &element)
{
    for(const Song &song : element.songs)
    {
        playQueue.push({&element,&song});
    }
}
void rotateQueue(queue<QueueEntry> &playQueue,size_t count)
{
    for(size_t i=0;i<count;i++)
    {
        QueueEntry entry=playQueue.front();
        playQueue.pop();
        playQueue.push(entry);
    }
}
void fillSequential(const CircularDLL<PlaylistElement> &playlist,queue<QueueEntry> &playQueue)
{
    const list<PlaylistElement> &elements=playlist.getElements();
    auto start=elements.begin();
    while(&*start!=playlist.getCurrent())
    {
        ++start;
    }
    auto cur=start;
    do
    {
        addGroup(playQueue,*cur);
        ++cur;
        if(cur==elements.end())
        {
            cur=elements.begin();
        }
    }
    while(cur!=start);
}
void fillShuffle(const CircularDLL<PlaylistElement> &playlist,queue<QueueEntry> &playQueue,mt19937 &engine)
{
    size_t groupCount=0;
    for(const PlaylistElement &element : playlist.getElements())
    {
        uniform_int_distribution<size_t> choosePosition(0,groupCount);
        size_t position=choosePosition(engine);
        size_t oldSongCount=playQueue.size();
        size_t prefixSongCount=0;
        for(size_t i=0;i<position;i++)
        {
            size_t songs=playQueue.front().element->songs.size();
            rotateQueue(playQueue,songs);
            prefixSongCount+=songs;
        }
        addGroup(playQueue,element);
        rotateQueue(playQueue,oldSongCount-prefixSongCount);
        groupCount++;
    }
}
QueueEntry findSong(const CircularDLL<PlaylistElement> &playlist,size_t index)
{
    for(const PlaylistElement &element : playlist.getElements())
    {
        size_t count=element.songs.size();
        if(index<count)
        {
            auto cur=findListNode(element.songs,index+1);
            return {&element,&*cur};
        }
        index-=count;
    }
    return {nullptr,nullptr};
}
bool simulatePlayback(const CircularDLL<PlaylistElement> &playlist,PlaybackMode mode,
    size_t count,ostream &output,mt19937 &engine,string &error)
{
    if(count==0)
    {
        error="Dainu kiekis turi buti teigiamas.";
        return false;
    }
    if(mode!=PlaybackMode::Sequential && mode!=PlaybackMode::Random && mode!=PlaybackMode::Shuffle)
    {
        error="Neteisingas grojimo rezimas.";
        return false;
    }
    if(playlist.isEmpty())
    {
        error="Grojarastis tuscias.";
        return false;
    }
    size_t totalSongs=0;
    bool selectedFound=false;
    for(const PlaylistElement &element : playlist.getElements())
    {
        if(element.songs.empty())
        {
            error="Grojarascio elementas neturi dainu.";
            return false;
        }
        if((element.type!=PlaylistElementType::Song && element.type!=PlaylistElementType::Album) ||
            (element.type==PlaylistElementType::Song && element.songs.size()!=1))
        {
            error="Neteisinga grojarascio elemento sandara.";
            return false;
        }
        if(element.songs.size()>numeric_limits<size_t>::max()-totalSongs)
        {
            error="Per didelis bendras dainu kiekis.";
            return false;
        }
        totalSongs+=element.songs.size();
        if(&element==playlist.getCurrent())
        {
            selectedFound=true;
        }
    }
    if(mode==PlaybackMode::Sequential && !selectedFound)
    {
        error="Nera pasirinkto grojarascio elemento.";
        return false;
    }
    if(!output.good())
    {
        error="Nepavyko isvesti grojimo sekos.";
        return false;
    }
    try
    {
        queue<QueueEntry> playQueue;
        output << "GROJIMO REZIMAS: ";
        if(mode==PlaybackMode::Sequential)
        {
            output << "is eiles" << endl;
            fillSequential(playlist,playQueue);
        }
        else if(mode==PlaybackMode::Random)
        {
            output << "random" << endl;
        }
        else
        {
            output << "shuffle" << endl;
        }
        uniform_int_distribution<size_t> chooseSong(0,totalSongs-1);
        size_t played=0;
        while(played<count)
        {
            if(!output.good())
            {
                error="Nepavyko isvesti grojimo sekos.";
                return false;
            }
            if(playQueue.empty())
            {
                if(mode==PlaybackMode::Shuffle)
                {
                    fillShuffle(playlist,playQueue,engine);
                }
                else
                {
                    playQueue.push(findSong(playlist,chooseSong(engine)));
                }
            }
            QueueEntry entry=playQueue.front();
            playQueue.pop();
            const Song &song=*entry.song;
            played++;
            output << played << ". " << (song.artist.empty()?"nenurodytas":song.artist)
                << " - " << (song.title.empty()?"nenurodytas":song.title)
                << " | Albumas: " << (song.album.empty()?"nenurodytas":song.album) << endl;
            if(mode==PlaybackMode::Sequential)
            {
                playQueue.push(entry);
            }
        }
        if(!output.good())
        {
            error="Nepavyko isvesti grojimo sekos.";
            return false;
        }
        error.clear();
        return true;
    }
    catch(const ios_base::failure&)
    {
        error="Nepavyko isvesti grojimo sekos.";
        return false;
    }
}
//Sarasas ir konsoles menu
void display(const list<Song> &songs)
{
    if(songs.empty())
    {
        cout << "Dainu sarasas tuscias." << endl;
        return;
    }
    cout << "Dainu kiekis: " << songs.size() << endl;
    int position=1;
    for(const Song &song : songs)
    {
        cout << position << ". " << (song.title.empty()?"nenurodytas":song.title) << endl;
        cout << "    Atlikejas: " << (song.artist.empty()?"nenurodytas":song.artist) << endl;
        cout << "    Albumas: " << (song.album.empty()?"nenurodytas":song.album) << endl;
        cout << "    Numeris albume: ";
        if(song.trackNumber==0)
        {
            cout << "nenurodytas" << endl;
        }
        else
        {
            cout << song.trackNumber << endl;
        }
        cout << "    Kelias: " << song.path << endl;
        position++;
    }
}
void displayElement(const PlaylistElement &element)
{
    cout << "Elemento tipas: ";
    if(element.type==PlaylistElementType::Song)
    {
        cout << "paviene daina" << endl;
    }
    else
    {
        cout << "albumas" << endl;
    }
    display(element.songs);
}
void displayPlaylist(const CircularDLL<PlaylistElement> &playlist)
{
    if(playlist.isEmpty())
    {
        cout << "Grojarastis tuscias." << endl;
        return;
    }
    cout << "Grojarascio elementu kiekis: " << playlist.size() << endl;
    int position=1;
    for(const PlaylistElement &element : playlist.getElements())
    {
        cout << endl << position << " ELEMENTAS" << endl;
        displayElement(element);
        position++;
    }
}
void displayCurrentElement(const CircularDLL<PlaylistElement> &playlist)
{
    const PlaylistElement *current=playlist.getCurrent();
    if(current==nullptr)
    {
        cout << "Nera pasirinkto elemento." << endl;
        return;
    }
    if(current->songs.empty())
    {
        cout << "Pasirinktas elementas neturi dainu." << endl;
        return;
    }
    const Song &song=current->songs.front();
    if(current->type==PlaylistElementType::Album)
    {
        cout << "Albumas: " << song.album << " (" << song.artist << ")" << endl;
        return;
    }
    cout << (song.title.empty()?"nenurodytas":song.title) << endl;
}
bool addSongFromFile(CircularDLL<PlaylistElement> &playlist,const string &path,string &error)
{
    Song song;
    if(!readSong(path,song,error))
    {
        return false;
    }
    PlaylistElement element;
    element.songs.push_back(move(song));
    playlist.addNodeEnd(move(element));
    return true;
}
bool addAlbumFromFolder(CircularDLL<PlaylistElement> &playlist,const string &path,string &error)
{
    PlaylistElement album;
    if(!readAlbum(path,album,error))
    {
        return false;
    }
    playlist.addNodeEnd(move(album));
    return true;
}
bool deleteSong(CircularDLL<PlaylistElement> &playlist,size_t elementPosition,size_t songPosition,string &error)
{
    PlaylistElement *element=playlist.getNodePosition(elementPosition);
    if(element==nullptr)
    {
        error="Neteisinga grojarascio elemento pozicija.";
        return false;
    }
    size_t count=element->songs.size();
    if(songPosition<1 || songPosition>count)
    {
        error="Neteisinga dainos pozicija pasirinktame elemente.";
        return false;
    }
    auto cur=findListNode(element->songs,songPosition);
    element->songs.erase(cur);
    if(element->songs.empty())
    {
        playlist.deleteNodePosition(elementPosition);
    }
    error.clear();
    return true;
}
bool moveSong(CircularDLL<PlaylistElement> &playlist,size_t elementPosition,size_t songPosition,size_t newPosition,string &error)
{
    PlaylistElement *element=playlist.getNodePosition(elementPosition);
    if(element==nullptr)
    {
        error="Neteisinga grojarascio elemento pozicija.";
        return false;
    }
    size_t count=element->songs.size();
    if(songPosition<1 || songPosition>count)
    {
        error="Neteisinga dainos pozicija pasirinktame elemente.";
        return false;
    }
    if(element->type==PlaylistElementType::Song)
    {
        if(count!=1)
        {
            error="Pavienes dainos elementas turi tureti viena daina.";
            return false;
        }
        if(newPosition<1 || newPosition>playlist.size())
        {
            error="Neteisinga nauja grojarascio pozicija.";
            return false;
        }
        playlist.moveNodePosition(elementPosition,newPosition);
    }
    else
    {
        if(newPosition<1 || newPosition>count)
        {
            error="Neteisinga nauja dainos pozicija albume.";
            return false;
        }
        if(songPosition!=newPosition)
        {
            auto cur=findListNode(element->songs,songPosition);
            auto target=findListNode(element->songs,newPosition);
            if(songPosition<newPosition)
            {
                ++target;
            }
            element->songs.splice(target,element->songs,cur);
        }
    }
    error.clear();
    return true;
}
bool artistLess(const PlaylistElement &left,const PlaylistElement &right)
{
    const string &leftArtist=left.songs.front().artist;
    const string &rightArtist=right.songs.front().artist;
    bool leftUnknown=isBlank(leftArtist);
    bool rightUnknown=isBlank(rightArtist);
    if(leftUnknown!=rightUnknown)
    {
        return !leftUnknown;
    }
    if(leftUnknown)
    {
        return false;
    }
    return leftArtist<rightArtist;
}
bool trackLess(const Song &left,const Song &right)
{
    if(left.trackNumber==0)
    {
        return false;
    }
    if(right.trackNumber==0)
    {
        return true;
    }
    return left.trackNumber<right.trackNumber;
}
bool sortPlaylistByArtist(CircularDLL<PlaylistElement> &playlist,string &error)
{
    for(const PlaylistElement &element : playlist.getElements())
    {
        if(element.songs.empty())
        {
            error="Rusiavimas negalimas: grojarascio elementas neturi dainu.";
            return false;
        }
    }
    playlist.quickSort(artistLess);
    error.clear();
    return true;
}
bool sortAlbumByTrack(CircularDLL<PlaylistElement> &playlist,size_t elementPosition,string &error)
{
    PlaylistElement *element=playlist.getNodePosition(elementPosition);
    if(element==nullptr)
    {
        error="Neteisinga grojarascio elemento pozicija.";
        return false;
    }
    if(element->type!=PlaylistElementType::Album)
    {
        error="Pasirinktas elementas nera albumas.";
        return false;
    }
    if(element->songs.empty())
    {
        error="Pasirinktas albumas neturi dainu.";
        return false;
    }
    quickSortList(element->songs,trackLess);
    error.clear();
    return true;
}
bool parsePosition(const string &text,size_t &position)
{
    string input=trim(text);
    if(input.empty())
    {
        return false;
    }
    size_t value=0;
    const size_t maximum=numeric_limits<size_t>::max();
    for(char symbol : input)
    {
        if(symbol<'0' || symbol>'9')
        {
            return false;
        }
        size_t digit=symbol-'0';
        if(value>(maximum-digit)/10)
        {
            return false;
        }
        value=value*10+digit;
    }
    if(value==0)
    {
        return false;
    }
    position=value;
    return true;
}
void displayMenu()
{
    cout << endl << endl << "MUZIKOS GROTUVAS" << endl;
    cout << "1 - Parodyti grojarasti" << endl;
    cout << "2 - Prideti viena daina" << endl;
    cout << "3 - Kitas grojarascio elementas" << endl;
    cout << "4 - Ankstesnis grojarascio elementas" << endl;
    cout << "5 - Prideti albuma is aplanko" << endl;
    cout << "6 - Pasalinti daina" << endl;
    cout << "7 - Pakeisti dainos vieta" << endl;
    cout << "8 - Surusiuoti grojarasti pagal atlikejus" << endl;
    cout << "9 - Surusiuoti albumo dainas pagal numeri" << endl;
    cout << "10 - Groti is eiles" << endl;
    cout << "11 - Groti random rezimu" << endl;
    cout << "12 - Groti shuffle rezimu" << endl;
    cout << "13 - Issaugoti grojarasti i JSON" << endl;
    cout << "14 - Ikelti grojarasti is JSON" << endl;
    cout << "0 - Iseiti" << endl;
    cout << "Pasirinkimas: ";
}
bool readInput(const string &prompt,string &text)
{
    cout << prompt;
    if(getline(cin,text))
    {
        return true;
    }
    cout << endl << "Ivestis baigta." << endl;
    return false;
}
bool readPosition(const string &prompt,size_t &position)
{
    string text;
    if(!readInput(prompt,text))
    {
        return false;
    }
    position=0;
    parsePosition(text,position);
    return true;
}
int main()
{
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif
    CircularDLL<PlaylistElement> playlist;
    mt19937 randomEngine(random_device{}());
    displayPlaylist(playlist);
    while(true)
    {
        displayMenu();
        string choice;
        if(!readInput("",choice))
        {
            break;
        }
        if(choice=="0")
        {
            cout << "Programa baigta." << endl;
            break;
        }
        string error;
        if(choice=="1")
        {
            displayPlaylist(playlist);
        }
        else if(choice=="3" || choice=="4")
        {
            if(choice=="3")
            {
                playlist.next();
            }
            else
            {
                playlist.previous();
            }
            displayCurrentElement(playlist);
        }
        else if(choice=="2" || choice=="5" || choice=="13" || choice=="14")
        {
            string prompt="Iveskite MP3 arba FLAC failo kelia: ";
            if(choice=="5") prompt="Iveskite albumo aplanko kelia: ";
            if(choice=="13") prompt="Iveskite issaugomo JSON failo kelia: ";
            if(choice=="14") prompt="Iveskite ikeliamo JSON failo kelia (pakeis dabartini grojarasti): ";
            string path;
            if(!readInput(prompt,path))
            {
                break;
            }
            bool success=false;
            string message;
            if(choice=="2")
            {
                success=addSongFromFile(playlist,path,error);
                message="Daina prideta.";
            }
            else if(choice=="5")
            {
                success=addAlbumFromFolder(playlist,path,error);
                message="Albumas pridetas.";
            }
            else if(choice=="13")
            {
                success=savePlaylist(playlist,path,error);
                message="Grojarastis issaugotas.";
            }
            else
            {
                success=loadPlaylist(playlist,path,error);
                message="Grojarastis ikeltas.";
            }
            if(success)
            {
                cout << message << endl;
                if(choice=="14") displayPlaylist(playlist);
            }
        }
        else if(choice=="6" || choice=="7" || choice=="8" || choice=="9" || choice=="10" || choice=="11" || choice=="12")
        {
            if(playlist.isEmpty())
            {
                cout << "Grojarastis tuscias." << endl;
                continue;
            }
            if(choice=="10" || choice=="11" || choice=="12")
            {
                size_t count=0;
                if(!readPosition("Kiek dainu parodyti grojimo sekoje? ",count)) break;
                PlaybackMode mode=PlaybackMode::Sequential;
                if(choice=="11") mode=PlaybackMode::Random;
                if(choice=="12") mode=PlaybackMode::Shuffle;
                if(count==0)
                {
                    error="Iveskite teigiama dainu kieki.";
                }
                else
                {
                    simulatePlayback(playlist,mode,count,cout,randomEngine,error);
                }
            }
            else
            {
                if(choice=="8" || choice=="9") cout << "PRIES RUSIAVIMA" << endl;
                displayPlaylist(playlist);
                size_t elementPosition=0;
                if(choice!="8")
                {
                    string prompt="Iveskite grojarascio elemento pozicija: ";
                    if(choice=="9") prompt="Iveskite rusiuojamo albumo elemento pozicija: ";
                    if(!readPosition(prompt, elementPosition))
                    {
                        break;
                    }
                    if(elementPosition==0 || elementPosition>playlist.size())
                    {
                        cout << "Klaida: Neteisinga grojarascio elemento pozicija." << endl;
                        continue;
                    }
                }
                if(choice=="8" || choice=="9")
                {
                    bool success=false;
                    string message;
                    if(choice=="8")
                    {
                        success=sortPlaylistByArtist(playlist,error);
                        message="Grojarastis surusiuotas pagal atlikejus.";
                    }
                    else
                    {
                        success=sortAlbumByTrack(playlist,elementPosition,error);
                        message="Albumo dainos surusiuotos pagal numeri.";
                    }
                    if(success)
                    {
                        cout << message << endl << "PO RUSIAVIMO" << endl;
                        displayPlaylist(playlist);
                    }
                }
                else if(choice=="6")
                {
                    size_t songPosition=0;
                    if(!readPosition("Iveskite dainos pozicija elemente (ne numeri albume): ", songPosition))
                    {
                        break;
                    }
                    if(deleteSong(playlist,elementPosition,songPosition,error))
                    {
                        cout << "Daina pasalinta." << endl;
                    }
                }
                else
                {
                    PlaylistElement *element=playlist.getNodePosition(elementPosition);
                    size_t songPosition=1;
                    size_t oldPosition=elementPosition;
                    string prompt="Iveskite nauja grojarascio pozicija (po perkelimo): ";
                    if(element->type==PlaylistElementType::Album)
                    {
                        if(!readPosition("Iveskite dainos pozicija albume (ne numeri albume): ", songPosition))
                        {
                            break;
                        }
                        if(songPosition==0 || songPosition>element->songs.size())
                        {
                            cout << "Klaida: Neteisinga dainos pozicija pasirinktame elemente." << endl;
                            continue;
                        }
                        oldPosition=songPosition;
                        prompt="Iveskite nauja dainos pozicija albume (po perkelimo): ";
                    }
                    size_t newPosition=0;
                    if(!readPosition(prompt, newPosition))
                    {
                        break;
                    }
                    if(moveSong(playlist,elementPosition,songPosition,newPosition,error))
                    {
                        if(oldPosition==newPosition)
                        {
                            cout << "Daina jau yra sioje pozicijoje." << endl;
                        }
                        else
                        {
                            cout << "Daina perkelta." << endl;
                        }
                        displayPlaylist(playlist);
                    }
                }
                if(error.empty())
                {
                    cout << "Pasirinktas elementas: ";
                    displayCurrentElement(playlist);
                }
            }
        }
        else
        {
            cout << "Neteisingas pasirinkimas." << endl;
        }
        if(!error.empty())
        {
            cout << "Klaida: " << error << endl;
        }
    }
    return 0;
}
