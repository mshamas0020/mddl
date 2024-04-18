// syntax.hpp

#ifndef __MDDL_SYNTAX_HPP__
#define __MDDL_SYNTAX_HPP__

#include "data_ref.hpp"
#include "expr.hpp"
#include "utils.hpp"


#include "libremidi/libremidi.hpp"

#include <bitset>
#include <cstdint>
#include <list>
#include <vector>


namespace MDDL {

namespace MIDI = libremidi;

static constexpr int N_MIDI_NOTES           = 128;
static constexpr int FUNCTION_MIN_ID_LEN    = 3;
static constexpr int BRANCH_ID_LEN          = 2;
static constexpr int MELODY_MIN_ID_LEN      = 3;
static constexpr int SEQ_LITERAL_MIN_ID_LEN = 3;

// Concrete Syntax Tree
struct CST
{
    struct Node
    {
        Node() = default;
        ~Node() { delete child; delete sibling; }

        Node*   parent                  = nullptr;
        Node*   child                   = nullptr;
        Node*   sibling                 = nullptr;
        uint8_t note                    = 0;
        bool    held                : 1 = true;
        bool    outlives_ancestor   : 1 = false;
        bool    excl_bass           : 1 = false;
        bool    excl_chord          : 1 = false;
        bool    excl_melody         : 1 = false;
        bool    excl_staccato       : 1 = false;

        bool has_parent() const { return (parent != nullptr); }
        bool has_child() const { return (child != nullptr); }
        bool has_sibling() const { return (sibling != nullptr); }
    };

    CST() = default;

    void note_on( uint8_t note );
    void note_off( uint8_t note );

    void reset();

    void print() const;

    Node*   head    = nullptr;
    Node*   tail    = nullptr;

};

enum class SyntaxType
{
    UNKNOWN,
    FUNCTION_DEF,
    FUNCTION_CALL,
    BRANCH,
    OPERATOR,
    VARIABLE,
    VALUE_LITERAL,
    SEQUENCE_LITERAL,
    SEPARATOR,
    ERROR
};

// Abstract Syntax Tree
struct AST 
{
    struct Node
    {
        Node() = default;
        ~Node() { delete child; delete sibling; }
        SyntaxType  type        = SyntaxType::UNKNOWN;
        Node*       parent      = nullptr;
        Node*       child       = nullptr;
        Node*       sibling     = nullptr;
        Symbol      id          = "";
        uint8_t     note_start  = 0;

        bool has_parent() const { return (parent != nullptr); }
        bool has_child() const { return (child != nullptr); }
        bool has_sibling() const { return (sibling != nullptr); }
    };

    AST() = default;

    void build_from_cst( const CST& cst );
    Node* traverse_cst( const CST::Node* cst, Node* parent, uint8_t split );

    void set_ief_code( OpId code ) { ief_code = code; }

    void reset();

    void print() const;

    Node*   head        = nullptr;

    // ief code is set by vendor specific MIDI msg
    // and overrides function of operation with MIDI note val 0
    OpId    ief_code    = IEF_DEFAULT; 
    bool    error       = false;
};

    

class SyntaxParser
{
public:
    SyntaxParser() = default;

    void set_tempo( int bpm ) { tempo = bpm; }
    void set_ppq( int ticks ) { ppq = ticks; }

    void process_msg( const MIDI::message& msg, int64_t tick );

    bool get_all_notes_off() const { return notes_active.any(); }
    int note_count() { return notes_active.count(); }

    void note_on( uint8_t note, uint8_t vel, int64_t tick );
    void note_off( uint8_t note, int64_t tick );


    const AST& get_ast() { return ast; }
    bool pending_ast() const { return pending; }

    bool active_sltx() const { return sltx != nullptr; }
    void set_sltx( SeqLit* x ) { sltx = x; }
    void force_sltx() { sltx_forced = true; }
    void close_sltx();

    void clear();

    CST                 cst;
    AST                 ast;

private:
    void sltx_note_on( uint8_t note, uint8_t vel, int64_t tick );
    void sltx_note_off( uint8_t note, int64_t tick );

    std::bitset<N_MIDI_NOTES>
                        notes_active        = {};
    bool                pending             = false;
    OpId                ief_code            = IEF_DEFAULT; 

    SeqLit*             sltx                = nullptr;
    std::list<int64_t>  sltx_held           = {};   // idxs
    bool                sltx_forced         = false;
    int64_t             prev_note_on_tick   = 0;
    int64_t             prev_event_tick     = 0;

    int                 tempo               = 120;
    int                 ppq                 = 960;
};

} // namespace MDDL

#endif // __MDDL_SYNTAX_HPP__