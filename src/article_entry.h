#ifndef ARTICLE_ENTRY
# define ARTICLE_ENTRY

#include <boost/intrusive/slist.hpp>
#include <boost/intrusive/list.hpp>
#include <boost/tokenizer.hpp>
#include <boost/unordered_set.hpp>
#include <string>
#include <vector>
#include <algorithm>
#include <iostream>
#include "word.h"
#include "wn.h"

class article_entry;

class rel_article_entry : public boost::intrusive::slist_base_hook<> {
public:
    rel_article_entry(article_entry * payload) : boost::intrusive::slist_base_hook<>() {
        m_article = payload;
    }

    article_entry* payload() {
        return m_article;
    }

private:
    article_entry * m_article;
};

class article_entry : public boost::intrusive::list_base_hook<> {
public:
    article_entry() {}

    void set_url(std::string& url) {
        m_url = url;
    }

    void set_article(std::string& article) {
        // Naive way to split the words, should have done better
        typedef boost::tokenizer< boost::char_separator<char> > Tokenizer;
        boost::char_separator<char> sep("@./\' \";,:-!#$%^&*?><}{[]\\|_+=`~()");
        Tokenizer tok(article, sep);
        std::string t;
        for(Tokenizer::iterator beg=tok.begin(); beg!=tok.end();++beg){
            t = *beg;
            std::transform(t.begin(), t.end(), t.begin(), ::tolower);
            op_word* p = op_word::get_word(t);
            if (p) {
                m_words.push_back(p);
                m_uni_words.insert(p);
            }
        }
    }

    std::string & get_url() { return m_url; }

    std::string get_article() {
        std::string res;
        for(std::vector<op_word* >::iterator beg = m_words.begin(); beg != m_words.end(); ++beg){
            res += " ";
            res += (*beg)->get_word();
        }
        return res;
    }

    std::vector<op_word*>& get_words() {return m_words;}
    boost::unordered_set<op_word*>& get_uni_words() { return m_uni_words;}

private:
    std::string m_url;
    std::vector<op_word*> m_words;
    boost::unordered_set<op_word*> m_uni_words;
    boost::intrusive::slist<rel_article_entry> rel_articles; 
};

#endif // ARTICLE_ENTRY
