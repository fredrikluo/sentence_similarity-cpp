#ifndef WORD_H
# define WORD_H

#include "wn.h"
#include <string>
#include <vector>
#include <boost/unordered_map.hpp>
#include <boost/unordered_set.hpp>

class op_word {
public:
    typedef struct _syn_node {
        size_t sid;
#ifdef DEBUG
        std::string name;
#endif // DEBUG
        boost::unordered_set<size_t> syn_with_shared_words;
        std::vector<size_t> hypern;
    } op_syn_node;

    static void finalize(); 
    static void dump_word_map();
    void dump();

    const std::string& get_word() const { return m_word; }
    std::vector<op_syn_node* >& get_synsets() { return  m_synsets;}

    float get_info_content() const { return m_info_content; }
    int id() { return m_id;}

    //TODO: to move into another library in the future.
    static void loaddb(std::string& db_file);
    static op_word* get_word(std::string & word);

private:
    // 1 is first none prime number, use it for the default value of the id.
    op_word():m_info_content(0.0), m_id(1) {}
    bool load(std::string& word, float ic);
    void get_synsets(char *searchstr, std::vector<op_syn_node* >& res);
    void gethypens(SynsetPtr synptr, const int ptrtyp, int dbase, int depth, std::vector<size_t>& res);
    static op_word* get_instance(std::string& word, float ic, int id);

#ifdef DEBUG
    void test_word_similarity();
    const char* dumpsynset(const char *head, SynsetPtr synptr, const char *tail, int antflag, int markerflag);
    const char* dumpsynet_from_pos(size_t file_pos);
#endif //DEBUG

private:
    std::string m_word;
    float m_info_content;
    int m_id;
    std::vector<op_syn_node *> m_synsets;

    typedef boost::unordered_map<std::string, op_word* > word_map;
    static word_map s_word_map;

    typedef boost::unordered_map<size_t, op_syn_node* > node_map;
    static node_map s_node_map;
};

#endif // WORD_H
