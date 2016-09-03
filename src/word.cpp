/*

   search.c - WordNet library of search code

*/
#include "word.h"

#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <boost/algorithm/string.hpp>

#ifdef DEBUG
#define ALLWORDS	0	/* print all words */
#define SKIP_ANTS	0	/* skip printing antonyms in printsynset() */
#define PRINT_ANTS	1	/* print antonyms in printsynset() */
#define SKIP_MARKER	0	/* skip printing adjective marker */
#define PRINT_MARKER	1	/* print adjective marker */
#define DEFON 1
#define DEFOFF 0
#endif // DEBUG
using namespace std;

/* Forward function declarations */
extern "C" int depthcheck(int depth, SynsetPtr synptr);
extern "C" IndexPtr getindex(char *searchstr, int dbase);
extern "C" SynsetPtr read_synset(int dbase, long boffset, const char *word);
extern "C" void catword(char *, SynsetPtr, int, int, int);

op_word::node_map op_word::s_node_map;
op_word::word_map op_word::s_word_map;

op_word* op_word::get_word(std::string & word) { 
    if (s_word_map.find(word) != s_word_map.end()) {
        return s_word_map[word];
    }

    op_word * p = NULL;
    char * p_str = strdup(word.c_str());
    char * morph_word = NULL;
    if ((morph_word = morphstr(p_str, NOUN))) {
        do {
            word = morph_word;
            if (s_word_map.find(word) != s_word_map.end()) {
                p = s_word_map[word];
                break;
            }
        } while ((morph_word = morphstr(NULL, NOUN)));
    }

    if (!p) {
        p = get_instance(word, 1.0, 1);
        s_word_map[word] = p;
    }

    free(p_str);
    return p;
}

op_word* op_word::get_instance(std::string& word, float ic, int id){
    op_word* p = new op_word();
    p->load(word, ic);
    p->m_id = id;
    return p;
}

bool op_word::load(std::string& word, float ic) {
    m_word = word;
    m_info_content = ic;
    char * pword = strdup(m_word.c_str());
    get_synsets(pword, m_synsets);
    free(pword);
    return true;
}

void op_word::loaddb(std::string& db_file) {
    ifstream db(db_file.c_str());
    string line;
    vector<string> strs;
    while (std::getline(db, line)) {
        // split the word and value
        strs.clear();
        boost::split(strs,line,boost::is_any_of(" "));
        assert(strs.size() == 3 && "you have problem with the data file");
        string& word = strs[0];
        op_word * p = op_word::get_instance(word, stof(strs[1]), stoi(strs[2]));
        if (p) {
            s_word_map[word] = p;
        }
    }
}

void op_word::gethypens(SynsetPtr synptr, const int ptrtyp, int dbase, int depth, vector<size_t>& res) {
    int i;
    SynsetPtr cursyn;

    for (i = 0; i < synptr->ptrcount; i++) {
        if ((ptrtyp == HYPERPTR && (synptr->ptrtyp[i] == HYPERPTR || synptr->ptrtyp[i] == INSTANCE)) ||
                ((synptr->ptrtyp[i] == ptrtyp) && ((synptr->pfrm[i] == 0) || (synptr->pfrm[i] == synptr->whichword)))
           ) {
            /* Read synset pointed to */
            cursyn = read_synset(synptr->ppos[i], synptr->ptroff[i], "");
            res.push_back(cursyn->hereiam);

#ifdef DUMP_SYNSET
            dumpsynset("", cursyn, "", PRINT_ANTS, PRINT_MARKER);
#endif // DUMP_SYNSET

            if (depth) {
                depth = depthcheck(depth, cursyn);
                gethypens(cursyn, ptrtyp, getpos(cursyn->pos), (depth+1), res);
            }

            free_synset(cursyn);
        }
    }
}

void op_word::get_synsets(char *searchstr, vector<op_syn_node* >& res) {
    const int dbase = NOUN;
    const int depth = 1;

    SynsetPtr cursyn;
    IndexPtr idx = NULL;
    int i = 0;
    int offsetcnt = 0;
    unsigned long offsets[MAXSENSE];
    int skipit = 0;

    /* look at all spellings of word */
    while ((idx = getindex(searchstr, dbase)) != NULL) {
        searchstr = NULL;	/* clear out for next call to getindex() */

        /* Go through all of the searchword's senses in the
           database and perform the search requested. */

        for (int sense = 0; sense < idx->off_cnt; sense++) {
            /* Determine if this synset has already been done
               with a different spelling. If so, skip it. */
            for (i = 0, skipit = 0; i < offsetcnt && !skipit; i++) {
                if (offsets[i] == idx->offset[sense])
                    skipit = 1;
            }

            if (skipit != 1) {
                offsets[offsetcnt++] = idx->offset[sense];
                cursyn = read_synset(dbase, idx->offset[sense], idx->wd);
                op_syn_node * node;
                if (s_node_map.find(cursyn->hereiam) != s_node_map.end()) {
                    node = s_node_map[cursyn->hereiam];
                } else {
                    node = new op_syn_node();
                    node->sid = cursyn->hereiam;
                    gethypens(cursyn, HYPERPTR, dbase, depth, node->hypern);
                    s_node_map[cursyn->hereiam] = node;
                }

                res.push_back(node);
                free_synset(cursyn);
            }

        } /* end for (sense) */

        free_index(idx);
    } /* end while (idx) */

    /** all the synsets contain this word should also be connected */
    for (int i = 0; i < res.size(); i ++) {
        for (int j = 0; j < res.size(); j ++) {
            if (i != j) {
                if (res[i]->syn_with_shared_words.find(res[j]->sid) == res[i]->syn_with_shared_words.end()) {
                    res[i]->syn_with_shared_words.insert(res[j]->sid);
                }
            }
        }
    }
}

void op_word::finalize() {
    for (word_map::iterator itr = s_word_map.begin(); itr != s_word_map.end(); ++itr) {
            delete itr->second;
    }

    for (node_map::iterator itr = s_node_map.begin(); itr != s_node_map.end(); ++itr) {
            delete itr->second;
    }
}

#ifdef DEBUG
void op_word::dump() {
    printf("%s: %f ", m_word.c_str(), m_info_content);
    vector <op_syn_node* > & res = m_synsets;
    for (size_t i = 0; i < res.size() ; i ++) {
        printf("%lx:%lx", res[i]->sid, res[i]->syn_with_shared_words.size());
        for (vector<size_t>::iterator itr = res[i]->hypern.begin(); itr != res[i]->hypern.end(); itr ++) {
            printf("> %s", dumpsynet_from_pos(*itr));
        }
        printf(":");
    }
    printf("\n");
}

const char* op_word::dumpsynet_from_pos(size_t file_pos) {
    SynsetPtr sptr = read_synset(NOUN,file_pos, NULL);
    return dumpsynset("", sptr, "", PRINT_ANTS, PRINT_MARKER);
}

const char * op_word::dumpsynset(const char *head, SynsetPtr synptr, const char *tail, int antflag, int markerflag) {
    int i, wdcnt;
    static char tbuf[SMLINEBUF];

    memset(tbuf, 0, sizeof(tbuf));
    strcat(tbuf, head);		/* print head */
    sprintf(tbuf + strlen(tbuf),"--> {%ld} ", synptr->hereiam);

    for(i = 0, wdcnt = synptr->wcount; i < wdcnt; i++) {
        catword(tbuf, synptr, i, markerflag, antflag);
        if (i < wdcnt - 1)
            strcat(tbuf, ", ");
    }

    strcat(tbuf,tail);
    return tbuf;
}

void op_word::dump_word_map() {
    for (word_map::iterator itr = s_word_map.begin(); itr != s_word_map.end(); ++itr) {
        itr->second->dump();
    }
}
#endif //DEBUG

