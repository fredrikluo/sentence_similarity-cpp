#include "news_processor.h"
#include "word.h"
#include "article_similarity.h"
#include <boost/timer/timer.hpp>

void news_processor::load(std::string &text_file, std::string& db_file) {
    // construct the wordnet database
    {
        boost::timer::auto_cpu_timer t;
        op_word::loaddb(db_file);
    }

    // Load the article
    {
        boost::timer::auto_cpu_timer t;
        m_article_list = m_data_loader.load(text_file);
    }
    // load the info_content_database

    // calculate the similarity
    article_similarity as(*m_article_list);
    {
        boost::timer::auto_cpu_timer t;
        as.calc(0, 2000);
    }
}
