#include "data_loader.h"

#include <boost/iostreams/device/mapped_file.hpp>
#include <algorithm>
#include <ctype.h>

using namespace boost;
using namespace boost::intrusive;

data_loader::data_loader() {

}

shared_ptr<article_list> data_loader::load(const std::string& text_file) {
    iostreams::mapped_file mmap(text_file, iostreams::mapped_file::readonly);
    const char* data = mmap.const_data();
    size_t end_pos = mmap.size();
    size_t cur_pos = 0;

    shared_ptr<article_list> data_list(new article_list());
    while (data && cur_pos != end_pos) {
        article_entry* ae = get_next_article(data, cur_pos, end_pos);
        if (ae == NULL) {
            break;
        }

        // push it to a list
        data_list->add_article_entry(*ae);
    }

    return data_list;
}

article_entry * data_loader::get_next_article(const char * data, size_t& cur_pos, size_t end_pos) {
    // read a line
    static char TMPBUF[4096];
    size_t start_pos = cur_pos;
    size_t sentence_start = 0;
    size_t buf_pos = cur_pos - start_pos;

    for (;cur_pos < end_pos && buf_pos < 4096; cur_pos ++) {
        char c = data[cur_pos];
        buf_pos = cur_pos - start_pos;
        if (c == ';' && !sentence_start) {
            sentence_start = buf_pos + 1;
            TMPBUF[buf_pos] = 0;
        } else if (c != '\n' && cur_pos != end_pos - 1) {
            if (isalpha(c) || !sentence_start) {
                TMPBUF[buf_pos] = c;
            } else {
                TMPBUF[buf_pos] = ' ';
            }
        } else {
            TMPBUF[buf_pos] = 0;
            assert(sentence_start > 0 && "we can't find sentence start in the line");
            article_entry * ae = new article_entry();
            ae->set_url( * (new std::string(TMPBUF)));
            ae->set_article( * (new std::string(TMPBUF + sentence_start)));
            cur_pos ++;
            return ae;
        }
    }

    return NULL;
}

