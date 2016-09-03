#ifndef DATA_LOADER_H
# define DATA_LOADER_H

#include <string>

#include "article_entry.h"
#include "article_list.h"

#include <boost/shared_ptr.hpp>
#include <boost/intrusive/list.hpp>

class data_loader {
public:
    data_loader();
    boost::shared_ptr<article_list> load(const std::string & text);

private:
    article_entry * get_next_article(const char * data, size_t& cur_pos, size_t end_pos);
};

#endif // DATA_LOADER_H

