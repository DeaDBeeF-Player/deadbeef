#ifndef BML_PARSER_H
#define BML_PARSER_H

class Bml_Parser
{
    struct Bml_Node {
        char *key;
        char *value;
        struct Bml_Node *next;
    };

    Bml_Node *nodes;
    Bml_Node *tail;

    void addNode (const char *key, const char *value);
    Bml_Node *walkToNode (const char *path) const;

public:
    Bml_Parser();
    ~Bml_Parser();

    void clearDocument ();

    void parseDocument(const char * document, size_t max_length = ~0UL);

    const char * enumValue(const char *path) const;
    
    void setValue(const char *path, long value);
    void setValue(const char *path, const char * value);

    void serialize(char *out, int buffer_size) const;
};

#endif // BML_PARSER_H
