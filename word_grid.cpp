// Copyright 2017 Matthew Chandler

// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include <algorithm>
#include <array>
#include <fstream>
#include <iostream>
#include <optional>
#include <string>
#include <tuple>
#include <unordered_set>
#include <vector>

#include <cstring>

#include <getopt.h>

const int ALPHABET_LEN = 26;

struct Args
{
    bool use_apostrophe = true;
    bool restrict_small_words = true;
    std::string dictionary_filename = "/usr/share/dict/words";
    int width = 0;
    int height = 0;
};

std::optional<Args> parse_arguments(int argc, char ** argv)
{
    Args args;

    option longopts[] =
    {
        {"help", no_argument, NULL, 'h'},
        {"dictionary", required_argument, NULL, 'd'},
        {"no-apostrophe", no_argument, NULL, 'n'},
        {"small-words", no_argument, NULL, 's'}
    };

    int opt = 0;
    extern char * optarg;
    extern int optind, optopt;

    std::string prog_name = argv[0];
    auto sep_pos = prog_name.find_last_of("/");
    if(sep_pos != std::string::npos)
        prog_name = prog_name.substr(sep_pos + 1);

    auto usage = "usage: " + prog_name + " [-h] [-n] [-s] WIDTH HEIGHT\n";

    int ind = 0;
    while((opt = getopt_long(argc, argv, ":hd:sn", longopts, &ind)) != -1)
    {
        switch(opt)
        {
            case 'd':
                args.dictionary_filename = optarg;
                break;
            case 'n':
                args.use_apostrophe = false;
                break;
            case 's':
                args.restrict_small_words = false;
                break;
            case 'h':
                std::cout<<usage<<"\n";
                std::cout<<
                    "Word grid generator\n\n"
                    "Positional arguments:\n"
                    "  WIDTH HEIGHT          Width and height of grid to generate.\n"
                  u8"                        Width × Height must be ≤ "<<ALPHABET_LEN<<"\n\n"
                    "Optional arguments\n"
                    "  -h, --help            Show this help message and exit\n"
                    "  -n, --no-apostrophe   Don't generate words with apostrophes\n"
                  u8"  -s  --small-words     Dont' restrict small (≤ 2 letters) to\n"
                    "                        internally defined list\n"
                    " --dictionary DICTIONARY\n"
                    "  -d DICTIONARY         Dictionary file (defaults to /usr/share/dict/words)\n";
                return std::nullopt;
            case ':':
                std::cerr<<"Argument required for "<<(char)optopt<<"\n";
                std::cerr<<usage;
                return std::nullopt;
            case '?':
            default:
                std::cerr<<"Unknown option for "<<(char)optopt<<"\n";
                std::cerr<<usage;
                return std::nullopt;
        }
    }

    if(argc - optind < 2)
    {
        std::cerr<<"Missing arguments\n";
        std::cerr<<usage;
        return std::nullopt;
    }

    if(argc - optind > 2)
    {
        std::cerr<<"Too many arguments\n";
        std::cerr<<usage;
        return std::nullopt;
    }

    auto convert_dim = [](auto & dim, auto & name)-> auto
    {
        try
        {
            return std::make_optional(std::stoi(dim));
        }
        catch(std::invalid_argument &e)
        {
            std::cerr<<"Invalid integer for "<<name<<" argument: "<<dim<<"\n";
        }
        catch(std::out_of_range &e)
        {
            std::cerr<<"Value too large for "<<name<<" argument: "<<dim<<"\n";
        }
        return std::optional<int>();
    };

    auto width = convert_dim(argv[optind], "width");
    auto height = convert_dim(argv[optind + 1], "height");

    if(!width || !height)
        return std::nullopt;

    args.width = *width;
    args.height = *height;

    if(args.width <= 0)
    {
        std::cerr<<"Width is too small. Must be > 0\n";
        return std::nullopt;
    }

    if(args.height <= 0)
    {
        std::cerr<<"Height is too small. Must be > 0\n";
        return std::nullopt;
    }

    if(args.width * args.height > ALPHABET_LEN)
    {
        std::cerr<<u8"Width × Height is too large. Must be ≤ 26\n";
        return std::nullopt;
    }

    return std::make_optional(args);
}

std::optional<std::tuple<std::vector<std::string>, std::vector<std::unordered_set<std::string>>>>
get_word_lists(const Args & args)
{
    std::ifstream dictionary(args.dictionary_filename);
    try
    {
        dictionary.exceptions(std::ifstream::failbit | std::ifstream::badbit); // throw on error OR failure
    }
    catch(std::system_error & e)
    {
        std::cerr<<"Error opening "<<args.dictionary_filename<<": "<<std::strerror(errno)<<std::endl;
        return std::nullopt;
    }

    try
    {
        dictionary.exceptions(std::ifstream::badbit); // only throw on error

        std::unordered_set<std::string> row_words;
        std::unordered_set<std::string> col_words;

        std::string word;
        while(std::getline(dictionary, word, '\n'))
        {
            if(args.use_apostrophe)
                word.erase(std::remove(word.begin(), word.end(), '\''), word.end());

            std::vector<char> seen;

            bool skip_word = false;
            for(auto &c: word)
            {
                c = std::toupper(c);
                if(c < 'A' || c > 'Z')
                {
                    skip_word = true;
                    break;
                }

                if(std::find(seen.begin(), seen.end(), c) != seen.end())
                {
                    skip_word = true;
                    break;
                }
                seen.push_back(c);
            }
            if(skip_word)
                continue;

            const std::unordered_set<std::string> legal_small_words
            {
                "A", "I",
                "AH", "AM", "AN", "AS", "AT", "BE", "BY", "DC", "DO",
                "DR", "EX", "GO", "HA", "HE", "HI", "HO", "IF", "IN", "IS",
                "IT", "LA", "LO", "MA", "ME", "MR", "MS", "MY", "NO", "OF",
                "OH", "OK", "ON", "OR", "OW", "OX", "PA", "PI", "SO", "ST",
                "TO", "UP", "US", "WE"
            };

            if(args.restrict_small_words && word.size() <= 2 && !legal_small_words.count(word))
                continue;

            if(static_cast<int>(word.size()) == args.width)
                row_words.insert(word);
            if(static_cast<int>(word.size()) == args.height)
                col_words.insert(word);
        }

        // put row words into a sorted list
        std::vector<std::string>row_words_list(row_words.begin(), row_words.end());
        std::sort(row_words_list.begin(), row_words_list.end());

        // build list of 1,2,…,height -1, height col prefixes to check against
        std::vector<std::unordered_set<std::string>> col_prefixes(args.height);
        for(const auto & col: col_words)
        {
            for(std::size_t i = 0; i < static_cast<std::size_t>(args.height); ++i)
                col_prefixes[i].insert(col.substr(0, i + 1));
        }

        return std::make_optional(std::make_tuple(row_words_list, col_prefixes));
    }
    catch(std::system_error & e)
    {
        std::cerr<<"Error reading "<<args.dictionary_filename<<": "<<std::strerror(errno)<<std::endl;
        return std::nullopt;
    }
}

void find_grids(const std::vector<std::string> & word_list,
                const std::vector<std::unordered_set<std::string>> & col_prefixes,
                const int height,
                const std::vector<std::string> & rows = {})
{
    // On the last ro
    // try each word to see if it will fit
    // TODO: parallelize?
    for(const auto & word: word_list)
    {
        // check to see if adding this word would fit prefixes
        auto match = true;
        for(std::size_t i = 0; i < word.size(); ++i)
        {
            std::string col;
            for(const auto & row: rows)
                col += row[i];
            col += word[i];

            if(!col_prefixes[col.size() - 1].count(col))
            {
                match = false;
                break;
            }
        }

        if(!match)
            continue;

        // if this is the last row, print, continue
        if(static_cast<int>(rows.size()) == height - 1)
        {
            for(const auto & row: rows)
                std::cout<<row<<"\n";
            std::cout<<word<<"\n"<<std::endl;
            continue;
        }

        // generate new list of words, removing any that share a letter with this one
        auto next_word_list = word_list;

        // TODO: parallelize this too?
        next_word_list.erase(std::remove_if(next_word_list.begin(), next_word_list.end(),
                    [&word](const std::string & try_word)
                    {return std::find_first_of(try_word.begin(), try_word.end(), word.begin(), word.end()) != try_word.end();}),
                next_word_list.end());

        auto next_rows = rows;
        next_rows.push_back(word);

        // continue next row with newly reduced list
        find_grids(next_word_list, col_prefixes, height, next_rows);
    }
}

int main(int argc, char ** argv)
{
    auto args = parse_arguments(argc, argv);
    if(!args)
        return EXIT_FAILURE;

    auto words = get_word_lists(*args);
    if(!words)
        return EXIT_FAILURE;

    auto [row_words, col_prefixes] = *words;

    find_grids(row_words, col_prefixes, args->height);

    return EXIT_SUCCESS;
}
