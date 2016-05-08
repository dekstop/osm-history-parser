#include <unordered_set>
#include <cstdint>
#include <iostream>
#include <fstream>

#include <osmium/io/any_input.hpp>
#include <osmium/handler.hpp>
#include <osmium/visitor.hpp>
#include <osmium/osm/tag.hpp>
#include <osmium/tags/taglist.hpp>

struct HistoryHandler : public osmium::handler::Handler {

protected:
    std::unordered_set<osmium::user_id_type> m_filter_users;
    std::ofstream &m_nodefile, &m_wayfile, &m_relfile;

    bool matches_user_filter(const osmium::OSMObject& obj) {
        if (obj.user_is_anonymous()) return false;
        std::unordered_set<osmium::user_id_type>::const_iterator it = m_filter_users.find(obj.uid());
        if (it == m_filter_users.end()) return false;
        return true;
    }

    // http://stackoverflow.com/questions/1494399/how-do-i-search-find-and-replace-in-a-standard-string
    std::string ReplaceString(std::string subject, const std::string& search,
        const std::string& replace) {

        size_t pos = 0;
        while((pos = subject.find(search, pos)) != std::string::npos) {
            subject.replace(pos, search.length(), replace);
            pos += replace.length();
        }
        return subject;
    }

    std::string escape(const char* str) {
        return ReplaceString(ReplaceString(ReplaceString(ReplaceString(str, "\\", "\\\\"), "\n", "\\n"), "\t", "\\t"), "\r", "\\r");
    }

public:
    uint64_t nodes = 0, unodes = 0;
    uint64_t ways = 0, uways = 0;
    uint64_t rels = 0, urels = 0;

    HistoryHandler(std::unordered_set<osmium::user_id_type> &filter_users,
            std::ofstream &nodefile, std::ofstream &wayfile, std::ofstream &relfile) : Handler(),
        m_filter_users(filter_users), m_nodefile(nodefile), m_wayfile(wayfile), m_relfile(relfile) {
    }

    void node(const osmium::Node& node) {
        ++nodes;
        if (!matches_user_filter(node)) return;
        ++unodes;
        for (const auto& tag : node.tags()) {
            m_nodefile <<
                node.id() << "\t" <<
                node.version() << "\t" <<
                escape(tag.key()) << "\t" <<
                escape(tag.value()) << std::endl;
        }
    }

    void way(const osmium::Way& way) {
        ++ways;
        if (!matches_user_filter(way)) return;
        ++uways;
        for (const auto& tag : way.tags()) {
            m_wayfile <<
                way.id() << "\t" <<
                way.version() << "\t" <<
                escape(tag.key()) << "\t" <<
                escape(tag.value()) << std::endl;
        }
    }

    void relation(const osmium::Relation& rel) {
        ++rels;
        if (!matches_user_filter(rel)) return;
        ++urels;
        for (const auto& tag : rel.tags()) {
            m_relfile <<
                rel.id() << "\t" <<
                rel.version() << "\t" <<
                escape(tag.key()) << "\t" <<
                escape(tag.value()) << std::endl;
        }
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
        std::cerr << "Usage: " << argv[0] << " OSM-HISTORY-FILE USERID_INFILE NODE_OUTFILE WAY_OUTFILE REL_OUTFILE" << std::endl;
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

    std::ofstream wayfile; 
    wayfile.open(argv[4]);

    std::ofstream relfile; 
    relfile.open(argv[5]);

    HistoryHandler handler(filter_users, nodefile, wayfile, relfile);
    osmium::apply(reader, handler);
    reader.close();
    nodefile.close();
    wayfile.close();
    relfile.close();

    std::cout << "Nodes: "     << handler.unodes << " of " << handler.nodes << "\n";
    std::cout << "Ways: "      << handler.uways  << " of " << handler.ways << "\n";
    std::cout << "Relations: " << handler.urels  << " of " << handler.rels << "\n";

    google::protobuf::ShutdownProtobufLibrary();
}
