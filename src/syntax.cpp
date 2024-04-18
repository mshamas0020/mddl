// syntax.cpp

#include "errors.hpp"
#include "syntax.hpp"

#include <format>
#include <iostream>
#include <string>
#include <span>



namespace MDDL {

static uint8_t MDDL_SYSEX_ID = 0x4d;

namespace MIDI = libremidi;

void CST::reset()
{
    delete head;
    head = nullptr;
    tail = nullptr;
}


void CST::note_on( uint8_t note )
{
    Node* node = new Node;
    node->note = note;

    if ( tail != nullptr ) {
        if ( tail->held ) {
            // make child

            node->parent = tail;
            tail->child = node;

            // tail cannot be staccato -- a note-on occurs while tail is held
            tail->excl_staccato = true;
            
            if ( tail->outlives_ancestor ) {
                // we must be in a melody, exclude all else
                // excl_bass's and excl_staccato's will be set by other guards
                // so set excl_chord until a held non-melody node is found
                node->excl_chord = true;
                tail->excl_chord = true;
                Node* ancestor = tail->parent;
                while ( ancestor != nullptr && !ancestor->held ) {
                    ancestor->excl_chord = true;
                    ancestor = ancestor->parent;
                }
            }

            Node* grandparent = tail->parent;
            if ( grandparent != nullptr && grandparent->held )
                // three notes must have been held at once, so exclude melody
                grandparent->excl_melody = true;
        } else {
            // make sibling

            Node* parent = tail->parent;
            node->parent = parent;
            tail->sibling = node;

            if ( parent != nullptr )
                // melody cannot have more than one child
                parent->excl_melody = true;
        }
    }

    if ( head == nullptr )
        head = node;

    tail = node;
}
    
void CST::note_off( uint8_t note )
{
    bool all_children_off = true;
    Node* node = tail;
    while ( node->note != note ) {

        if ( node->held ) {
            all_children_off = false;
            node->outlives_ancestor = true;
            // bass and staccato cannot outlive ancestor
            node->excl_bass = true;
            node->excl_staccato = true;
        }

        node = node->parent;

        sys_assert( node != nullptr );
    }

    node->held = false;

    if ( !node->has_child() || !all_children_off )
        // bass must have children
        // and must not be outlived by children
        node->excl_bass = true;

    if ( all_children_off ) {
        if ( !node->outlives_ancestor || node->has_child() )
            // melody node must either outlive an ancestor or a child
            node->excl_melody = true;

        while ( tail->has_parent() ) {
            if ( tail->parent->held )
                break;
            tail = tail->parent;
        }
    }
}

static std::string cst_node_to_str( const CST::Node* node, const CST::Node* tail )
{
    const int n = 0b1000 * !node->excl_bass
                + 0b0100 * !node->excl_chord
                + 0b0010 * !node->excl_melody
                + 0b0001 * !node->excl_staccato;

    return std::format( "{}{}{}-{:1x}",
        node == tail ? "T" : " ",
        node->held ? "O" : "X",
        note_to_str( node->note ), n );
}

void CST::print() const
{
    if ( head == nullptr ) {
        std::cout << "Empty\n";
        return;
    }

    char branch = 'A';

    std::vector<Node*> row = { head };
    std::vector<char> branch_ids = { branch };

    while ( true ) {
        bool active_cols = false;
        for ( int i = 0; i < (int )row.size(); i ++ ) {
            Node* node = row[i];
            char id = branch_ids[i];
            if ( node == nullptr ) {
                std::cout << "         ";
                continue;
            }

            std::cout << cst_node_to_str( node, tail ) << id << "  ";

            if ( node->sibling != nullptr ) {
                branch ++;
                row.insert( row.begin() + i + 1, node->sibling );
                branch_ids.insert( branch_ids.begin() + i + 1, branch );
            }

            row[i] = node->child;

            active_cols = true;
        }

        std::cout << "\n";

        if ( !active_cols )
            break;
    }
}



static Symbol notes_to_symbol( std::span<uint8_t> notes )
{
    Symbol s;
    s.resize( notes.size() );
    s[0] = (char )notes[0] % OCTAVE;

    for ( int i = 1; i < (int )notes.size(); i ++ )
        s[i] = (char )notes[i] - (char )notes[i - 1];

    return s;
}

static Symbol notes_to_symbol_sorted( std::span<uint8_t> notes )
{
    std::sort( notes.begin(), notes.end() );
    return notes_to_symbol( notes );
}

void AST::reset()
{
    delete head;
    head = nullptr;
    ief_code = IEF_DEFAULT;
    error = false;
}

void AST::build_from_cst( const CST& cst )
{
    reset();
    
    head = traverse_cst( cst.head, nullptr, 0 );
}

static bool cst_is_chord_start( const CST::Node* cst )
{
    return cst != nullptr
        && !cst->excl_chord
        && cst->has_child();
}

static bool cst_is_chord_extension( const CST::Node* cst )
{
    return cst != nullptr
        && !cst->excl_chord
        && !cst->has_sibling();
}

static bool cst_is_bass( const CST::Node* cst )
{
    return cst != nullptr
        && !cst->excl_bass
        && cst->has_child();
}

static bool cst_is_melody_start( const CST::Node* cst )
{
    return cst != nullptr
        && !cst->excl_melody
        && cst->has_child();
}

static bool cst_is_melody_continuation( const CST::Node* cst )
{
    return cst != nullptr
        && !cst->excl_melody
        && !cst->has_sibling();
}

static bool cst_is_staccato_above( const CST::Node* cst, uint8_t split )
{
    return cst != nullptr
        && !cst->excl_staccato
        && !cst->has_child()
        && cst->note > split;
}

static bool cst_is_staccato_below( const CST::Node* cst, uint8_t split )
{
    return cst != nullptr
        && !cst->excl_staccato
        && !cst->has_child()
        && cst->note <= split;
}

AST::Node* AST::traverse_cst( const CST::Node* cst, Node* parent, uint8_t split )
{
    if ( cst == nullptr )
        return nullptr;

    const bool is_root = (parent == nullptr);
    const CST::Node* cst_start = cst;
    const CST::Node* sibling = cst->sibling;
    const CST::Node* child = nullptr;
    const uint8_t split_start = split;

    Node* node = new Node;
    // size_t id_len = 1;
    node->note_start = cst->note;
    std::vector<uint8_t> notes = { cst->note };

    // check function def, function call, or branch statement
    if ( cst_is_chord_start( cst ) ) {
        split = std::max( split, cst->note );
        cst = cst->child;

        while ( cst_is_chord_extension( cst ) ) {
            // id_len ++;
            notes.push_back( cst->note );
            split = std::max( split, cst->note );
            cst = cst->child;
        }

        if ( (int )notes.size() >= FUNCTION_MIN_ID_LEN ) {
            // function call/def found
            node->id = notes_to_symbol_sorted( notes );
            node->type = (is_root && cst == nullptr)
                ? SyntaxType::FUNCTION_DEF
                : SyntaxType::FUNCTION_CALL;
            
            child = cst;
            goto exit;
        } else if ( is_root && (int )notes.size() == BRANCH_ID_LEN ) {
            node->id = notes_to_symbol_sorted( notes );
            node->type = SyntaxType::BRANCH;
            
            child = cst;
            goto exit;
        }

        // fell through, reset 
        cst = cst_start;
        split = split_start;
        notes.resize( 1 );
    }

    // check operator
    if ( cst_is_bass( cst ) ) {
        split = std::max( split, cst->note );
        cst = cst->child;

        node->id = notes_to_symbol( notes );
        node->type = SyntaxType::OPERATOR;
        child = cst;
        goto exit;
    }

    // check variable
    if ( cst_is_melody_start( cst ) ) {
        cst = cst->child;

        while ( cst_is_melody_continuation( cst ) ) {
            notes.push_back( cst->note );
            cst = cst->child;
        }

        node->id = notes_to_symbol( notes );

        // a variable cannot have a child
        if ( cst == nullptr ) {
            node->type = SyntaxType::VARIABLE;
        } else {
            node->type = SyntaxType::ERROR;
            error = true;
        }

        child = cst;
        goto exit;
    }

    // check value literal
    if ( cst_is_staccato_above( cst, split ) ) {
        cst = cst->sibling;

        while ( cst_is_staccato_above( cst, split ) ) {
            notes.push_back( cst->note );
            cst = cst->sibling;
        }

        node->id = notes_to_symbol( notes );
        node->type = SyntaxType::VALUE_LITERAL;
        
        sibling = cst;
        goto exit;
    }

    // check sequence literal
    if ( cst_is_staccato_below( cst, split ) ) {
        const uint8_t id_note = cst->note;
        cst = cst->sibling;

        while ( cst_is_staccato_below( cst, split ) && cst->note == id_note ) {
            notes.push_back( cst->note );
            cst = cst->sibling;
        }

        if ( (int )notes.size() >= SEQ_LITERAL_MIN_ID_LEN ) {
            node->id = notes_to_symbol( notes );
            node->type = SyntaxType::SEQUENCE_LITERAL;
            sibling = cst;
            goto exit;
        }

        // fell through, reset
        cst = cst_start;
        notes.resize( 1 );
    }

    // check separator
    if ( cst_is_staccato_below( cst, split ) ) {
        // ignore separator
        delete node;
        return traverse_cst( cst->sibling, parent, split_start );
    }

    node->type = SyntaxType::ERROR;
    error = true;

exit:
    if ( node->id.size() == 0 )
        node->type = SyntaxType::ERROR;
        
    if ( node->type != SyntaxType::ERROR )
        node->child = traverse_cst( child, node, split );

    node->sibling = traverse_cst( sibling, parent, split_start );

    return node;
}

static void print_ast_node( const AST& ast, const AST::Node* node )
{
    if ( node == nullptr )
        return;

    switch ( node->type ) {
        case SyntaxType::UNKNOWN: std::cout << "UNKNOWN"; break;
        case SyntaxType::FUNCTION_DEF: std::cout << "DEF"; break;
        case SyntaxType::FUNCTION_CALL: std::cout << "FN"; break;
        case SyntaxType::BRANCH: std::cout << "BR"; break;
        case SyntaxType::OPERATOR: std::cout << "OP"; break;
        case SyntaxType::VARIABLE: std::cout << "VAR"; break;
        case SyntaxType::VALUE_LITERAL: std::cout << "LIT"; break;
        case SyntaxType::SEQUENCE_LITERAL: std::cout << "SEQ"; break;
        default: std::cout << "ERROR"; break;
    }

    sys_assert( node->id.size() > 0 );

    std::cout << " " << symbol_to_str( node->id );

    if ( node->has_child() ) {
        std::cout << "( ";
        print_ast_node( ast, node->child );
        std::cout << " )";
    }

    if ( node->has_sibling() ) {
        std::cout << ", ";
        print_ast_node( ast, node->sibling );
    }
}

void AST::print() const
{
    print_ast_node( *this, head );
    std::cout << "\n";
}








void SyntaxParser::process_msg( const MIDI::message& msg, int64_t tick )
{
    const uint8_t note = msg.bytes[1] & 0x7F;
    const uint8_t vel = msg.bytes[2] & 0x7F;

    switch( msg.get_message_type() ) {
        case MIDI::message_type::NOTE_ON:
            if ( vel == 0 ) {
                note_off( note, tick );
                break;
            }
            note_on( note, vel, tick );
            break;
        case MIDI::message_type::NOTE_OFF:
            note_off( note, tick );
            break;
        case MIDI::message_type::SYSTEM_EXCLUSIVE:
            if ( msg.bytes.size() < 3 )
                break;
            if ( msg.bytes[1] == MDDL_SYSEX_ID )
                ief_code = (OpId )msg.bytes[2];
            break;
        default:
            break;
    }

    // std::cout << notes_active << "\n";

    // cst.print();
    // print_ast( ast );
}

void SyntaxParser::note_on( uint8_t note, uint8_t vel, int64_t tick )
{
    sys_assert( note < N_MIDI_NOTES );

    if ( notes_active[note] )
        return;
        
    notes_active[note] = 1;

    if ( active_sltx() ) {
        sltx_note_on( note, vel, tick );
        return;
    }
    
    // std::cout << "Note ON: " << note_to_str( note ) << "\n";

    cst.note_on( note );
}

void SyntaxParser::note_off( uint8_t note, int64_t tick )
{
    sys_assert( note < N_MIDI_NOTES );

    if ( !notes_active[note] )
        return;

    notes_active[note] = 0;
    
    if ( active_sltx() ) {
        sltx_note_off( note, tick );
        return;
    }
    
    // std::cout << "Note OFF: " << note_to_str( note ) << "\n";

    cst.note_off( note );

    /*
    AST dbg_ast;
    dbg_ast.build_from_cst( cst );
    dbg_ast.print();
    */

    if ( notes_active.none() ) {
        ast.build_from_cst( cst );
        ast.set_ief_code( ief_code );
        // ast.print();
        pending = true;
    }
}

void SyntaxParser::sltx_note_on( uint8_t note, uint8_t vel, int64_t tick )
{
    Sequence& seq = sltx->ref.get();

    std::lock_guard<std::mutex> guard( seq.mtx );

    const double ns_to_ticks = 1.0 / 1'000'000'000.f / 60.f * tempo * ppq;
    const int64_t hold = (int64_t )((tick - prev_event_tick) * ns_to_ticks);


    for ( int64_t idx : sltx_held )
        seq.note_hold( idx, hold );

    if ( note == sltx->note && !sltx_forced ) {
        close_sltx();
        return;
    }

    const int64_t wait = (seq.size == 0)
        ? 0
        : (int64_t )((tick - prev_note_on_tick) * ns_to_ticks);
    seq.note_on( note, vel, wait );
    sltx_held.push_back( seq.size - 1 );
    
    prev_note_on_tick = tick;
    prev_event_tick = tick;
}

void SyntaxParser::sltx_note_off( uint8_t note, int64_t tick )
{
    Sequence& seq = sltx->ref.get();

    std::lock_guard<std::mutex> guard( seq.mtx );

    const double ns_to_ticks = 1.0 / 1'000'000'000.f / 60.f * tempo * ppq;
    const int64_t hold = (int64_t )((tick - prev_event_tick) * ns_to_ticks);

    auto itr = sltx_held.begin();
    while ( itr != sltx_held.end() ) {
        const int64_t idx = *itr;
        seq.note_hold( idx, hold );

        if ( seq.at( idx ).pitch == note ) {
            itr = sltx_held.erase( itr );
            continue;
        }

        itr ++;
    }

    prev_event_tick = tick;
}

void SyntaxParser::close_sltx()
{
    sltx->ref.get().mark_complete();
    clear();
}

void SyntaxParser::clear()
{
    cst.reset();
    ast.reset(); 
    notes_active.reset();
    pending = false;
    ief_code = IEF_DEFAULT;

    sltx = nullptr;
    sltx_held.clear();
    sltx_forced = false;
    prev_note_on_tick = 0;
    prev_event_tick = 0;
}

} // namespace MDDL