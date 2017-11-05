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

#include <string>
#include <iostream>
#include <optional>

#include <getopt.h>

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
                    "                        Width × Height must be ≤ 26\n\n"
                    "Optional arguments\n"
                    "  -h, --help            Show this help message and exit\n"
                    "  -n, --no-apostrophe   Don't generate words with apostrophes\n"
                    "  -s  --small-words     Dont' restrict small (≤ 2 letters) to\n"
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

    return std::make_optional(args);
}

int main(int argc, char ** argv)
{
    auto args = parse_arguments(argc, argv);
    if(!args)
        return EXIT_FAILURE;

    return EXIT_SUCCESS;
}
