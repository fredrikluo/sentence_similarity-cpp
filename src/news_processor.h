#ifndef NEWS_PROCESSOR_H 
# define NEWS_PROCESSOR_H

#include <string>
#include "data_loader.h"
#include "article_list.h"

class news_processor {
public:
    void load(std::string & text_file, std::string& ic_file);

private:
    data_loader m_data_loader;
    boost::shared_ptr<article_list> m_article_list;
};

#endif // NEWS_PROCESSOR_H
