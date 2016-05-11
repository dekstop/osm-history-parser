#include <unordered_set>
#include <cstdint>
#include <iostream>
#include <fstream>

#include <osmium/io/any_input.hpp>
#include <osmium/handler.hpp>
#include <osmium/visitor.hpp>

struct HistoryHandler : public osmium::handler::Handler {

protected:
    std::unordered_set<osmium::object_id_type> m_filter_objects;
    std::ofstream &m_wayfile;

    bool matches_object_filter(const osmium::OSMObject& obj) {
        std::unordered_set<osmium::object_id_type>::const_iterator it = m_filter_objects.find(obj.id());
        if (it == m_filter_objects.end()) return false;
        return true;
    }

public:
    uint64_t ways = 0, uways = 0;

    HistoryHandler(std::unordered_set<osmium::object_id_type> &filter_objects, std::ofstream &wayfile) : Handler(),
        m_filter_objects(filter_objects), m_wayfile(wayfile) {
    }

    void node(const osmium::Node& node) {
    }

    void way(const osmium::Way& way) {
        ++ways;
        if (!matches_object_filter(way)) return;
        ++uways;
        if (way.visible()==false) {
            m_wayfile <<
                way.id() << "\t" <<
                way.version() << "\t" <<
                way.changeset() << "\t" <<
                way.timestamp().to_iso() << "\t" <<
                way.uid() << std::endl;
        }
    }

    void relation(const osmium::Relation& rel) {
    }
};

// http://insanecoding.blogspot.co.uk/2011/11/how-to-read-in-file-in-c.html
std::string get_file_contents(const std::string filename) {
    std::ifstream in(filename, std::ios::in | std::ios::binary);
    if (in) {
        std::string contents;
        in.seekg(0, std::ios::end);
        contents.resize(in.tellg());
        in.seekg(0, std::ios::beg);
        in.read(&contents[0], contents.size());
        in.close();
        return(contents);
    }
    throw(errno);
}

bool load_object_filter(const std::string filename, std::unordered_set<osmium::object_id_type> &filter_objects) {
    try {
        std::string str = get_file_contents(filename); // throws int errno
        std::istringstream ss(str);
        ss.imbue(std::locale::classic()); // locale independent parsing
        osmium::object_id_type uid;
        while (ss >> uid) { // throws std::exception
            filter_objects.insert(uid);
            if (ss.peek() == '\n') ss.ignore();
        }
        return true;
    }
    catch (int errno) {
        std::cout << "Error: " << strerror(errno) << std::endl;
    }
    catch (std::exception &e) {
        std::cout << "Exception: " << e.what() << std::endl;
    }
    return false;
}


int main(int argc, char* argv[]) {
    if (argc != 4) {
        std::cerr << "Usage: " << argv[0] << " OSM-HISTORY-FILE WAY_ID_INFILE WAY_OUTFILE" << std::endl;
        return 1;
    }

    osmium::io::File infile(argv[1]);
    osmium::io::Reader reader(infile);

    if (reader.header().has_multiple_object_versions()) {
        std::cout << "History file." << std::endl;
    }

    std::unordered_set<osmium::object_id_type> filter_objects;
    if (!load_object_filter(argv[2], filter_objects)) {
        return 1;
    }
    std::cout << "Loaded " << filter_objects.size() << " IDs to filter." << std::endl;

    std::ofstream wayfile; 
    wayfile.open(argv[3]);

    HistoryHandler handler(filter_objects, wayfile);
    osmium::apply(reader, handler);
    reader.close();
    wayfile.close();

    std::cout << "Ways: "      << handler.uways  << " of " << handler.ways << "\n";

    google::protobuf::ShutdownProtobufLibrary();
}

