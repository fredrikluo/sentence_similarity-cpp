#ifndef ARTICLE_LIST_H
# define ARTICLE_LIST_H

#include "article_entry.h"

class article_list {
public:
    void add_article_entry(article_entry& ae) {
        article_list.push_back(ae);
    }

    virtual ~article_list () {
        article_list.erase_and_dispose(article_list.begin(), article_list.end(), delete_disposer());
    }

    boost::intrusive::list<article_entry>& get_article_list() {
        return article_list;
    }

private:
    struct delete_disposer {
           void operator()(article_entry *delete_this)
                  {  delete delete_this;  }
    };

    boost::intrusive::list<article_entry>  article_list;
};

#endif // ARTICLE_LIST_H
