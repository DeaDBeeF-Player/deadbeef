#ifndef BML_PARSER_H
#define BML_PARSER_H

#include "blargg_common.h"

class Bml_Node
{
    char * name;
    char * value;
    Bml_Node *next;

    blargg_vector<Bml_Node> children;

    static Bml_Node emptyNode;

public:
    Bml_Node();
    Bml_Node(char const* name, size_t max_length = ~0UL);
    Bml_Node(Bml_Node const& in);

    ~Bml_Node();

    void clear();

    void setLine(const char * line, size_t max_length = ~0UL);
    Bml_Node& addChild(Bml_Node const& child);

    const char * getName() const;
    const char * getValue() const;
    
    void setValue(char const* value);

    size_t getChildCount() const;
    Bml_Node const& getChild(size_t index) const;

    Bml_Node & walkToNode( const char * path, bool use_indexes = false );
    Bml_Node const& walkToNode( const char * path ) const;
};

class Bml_Parser
{
    Bml_Node document;

public:
    Bml_Parser() { }

    void parseDocument(const char * document, size_t max_length = ~0UL);

    const char * enumValue(const char *path) const;
    
    void setValue(const char *path, long value);
    void setValue(const char *path, const char * value);

#if HAVE_SFM_METADATA
    void serialize(const char *out) const;
#endif
private:
#if HAVE_SFM_METADATA
    void serialize(std::ostringstream & out, Bml_Node const* node, unsigned int indent) const;
#endif
};

#endif // BML_PARSER_H
