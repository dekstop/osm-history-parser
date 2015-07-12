#include <unordered_map>
#include <cstdint>
#include <iostream>
#include <fstream>

#include <osmium/io/any_input.hpp>
#include <osmium/handler.hpp>
#include <osmium/visitor.hpp>

struct HistoryHandler : public osmium::handler::Handler {

protected:
    std::unordered_map<osmium::user_id_type, std::string> uid_name;
    std::ofstream &m_outfile;

    // http://stackoverflow.com/questions/1494399/how-do-i-search-find-and-replace-in-a-standard-string
    std::string ReplaceString(std::string subject, const std::string& search, const std::string& replace) const {
        size_t pos = 0;
        while((pos = subject.find(search, pos)) != std::string::npos) {
            subject.replace(pos, search.length(), replace);
            pos += replace.length();
        }
        return subject;
    }
    
    std::string escape(const std::string str) const {
        return ReplaceString(ReplaceString(ReplaceString(ReplaceString(str, "\\", "\\\\"), "\n", "\\n"), "\t", "\\t"), "\r", "\\r");
    }

    bool add_user(const osmium::user_id_type uid, const char* name) {
        std::unordered_map<osmium::user_id_type, std::string>::const_iterator it = uid_name.find(uid);
        if (it == uid_name.end()) {
            std::string str_name(name);
            uid_name[uid] = str_name;
            return true;
        }
        return false;
    }

public:
    uint64_t nodes = 0, unodes = 0;
    uint64_t ways = 0, uways = 0;
    uint64_t rels = 0, urels = 0;

    HistoryHandler(std::ofstream &outfile) : Handler(), m_outfile(outfile) {
    }

    void node(const osmium::Node& node) {
        ++nodes;
        if (node.user_is_anonymous()) return;
        ++unodes;
        add_user(node.uid(), node.user());
    }

    void way(const osmium::Way& way) {
        ++ways;
        if (way.user_is_anonymous()) return;
        ++uways;
        add_user(way.uid(), way.user());
    }

    void relation(const osmium::Relation& rel) {
        ++rels;
        if (rel.user_is_anonymous()) return;
        ++urels;
        add_user(rel.uid(), rel.user());
    }

    void save_map() const {
        for (auto it = uid_name.cbegin(); it != uid_name.cend(); ++it) {
            m_outfile <<
                it->first << "\t" <<
                escape(it->second) << std::endl;
        }
    }
};


int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " OSM-HISTORY-FILE OUTFILE" << std::endl;
        return 1;
    }

    osmium::io::File infile(argv[1]);
    osmium::io::Reader reader(infile);

    if (reader.header().has_multiple_object_versions()) {
        std::cout << "History file." << std::endl;
    }

    std::ofstream outfile; 
    outfile.open(argv[2]);

    HistoryHandler handler(outfile);
    osmium::apply(reader, handler);
    handler.save_map();
    reader.close();
    outfile.close();

    std::cout << "Nodes: "     << handler.unodes << " of " << handler.nodes << "\n";
    std::cout << "Ways: "      << handler.uways  << " of " << handler.ways << "\n";
    std::cout << "Relations: " << handler.urels  << " of " << handler.rels << "\n";

    google::protobuf::ShutdownProtobufLibrary();
}

