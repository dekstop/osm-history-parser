#include <unordered_set>
#include <cstdint>
#include <iostream>
#include <fstream>

#include <osmium/io/any_input.hpp>
#include <osmium/handler.hpp>
#include <osmium/visitor.hpp>

struct HistoryHandler : public osmium::handler::Handler {

protected:
    std::unordered_set<osmium::user_id_type> m_filter_users;
    std::ofstream &m_nodefile;

    bool matches_user_filter(const osmium::OSMObject& obj) {
        if (obj.user_is_anonymous()) return false;
        std::unordered_set<osmium::user_id_type>::const_iterator it = m_filter_users.find(obj.uid());
        if (it == m_filter_users.end()) return false;
        return true;
    }

public:
    uint64_t nodes = 0, unodes = 0;
    uint64_t ways = 0, uways = 0;

    HistoryHandler(std::unordered_set<osmium::user_id_type> &filter_users,
            std::ofstream &nodefile) : Handler(),
        m_filter_users(filter_users), m_nodefile(nodefile) {
    }

    void node(const osmium::Node& node) {
        ++nodes;
        if (!matches_user_filter(node)) return;
        ++unodes;
    }

    void way(const osmium::Way& way) {
        ++ways;
        if (!matches_user_filter(way)) return;
        ++uways;
        for (const auto& node : way.nodes()) {
            m_nodefile <<
                way.id() << "\t" <<
                way.version() << "\t" <<
                node.ref() << std::endl;
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

bool load_user_filter(const std::string filename, std::unordered_set<osmium::user_id_type> &filter_users) {
    try {
        std::string str = get_file_contents(filename); // throws int errno
        std::istringstream ss(str);
        ss.imbue(std::locale::classic()); // locale independent parsing
        osmium::user_id_type uid;
        while (ss >> uid) { // throws std::exception
            filter_users.insert(uid);
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
    if (argc != 6) {
        std::cerr << "Usage: " << argv[0] << " OSM-HISTORY-FILE USERID_INFILE WAY_NODE_OUTFILE" << std::endl;
        return 1;
    }

    osmium::io::File infile(argv[1]);
    osmium::io::Reader reader(infile);

    if (reader.header().has_multiple_object_versions()) {
        std::cout << "History file." << std::endl;
    }

    std::unordered_set<osmium::user_id_type> filter_users;
    if (!load_user_filter(argv[2], filter_users)) {
        return 1;
    }
    std::cout << "Loaded " << filter_users.size() << " user IDs to filter." << std::endl;

    std::ofstream nodefile; 
    nodefile.open(argv[3]);

    HistoryHandler handler(filter_users, nodefile);
    osmium::apply(reader, handler);
    reader.close();
    nodefile.close();

    std::cout << "Nodes: "     << handler.unodes << " of " << handler.nodes << "\n";
    std::cout << "Ways: "      << handler.uways  << " of " << handler.ways << "\n";

    google::protobuf::ShutdownProtobufLibrary();
}

