// This file contains a collection of quotes from famous scientists, 
// I added this quote collection from various sources on the internet.(A)
// The inspiration for this came from molecular dynamics tool called GROMACS which shows quotes during simulation runs. (coolstuff.cpp)

#pragma once
#include <string>
#include <vector>

struct ScientistQuote {
    std::string quote;
    std::string author;
};

static const std::vector<ScientistQuote> SCIENTIST_QUOTES = {
    {
        "An equation means nothing to me unless it expresses a thought of God.",
        "Srinivasa Ramanujan"
    },
    {
        "Be less curious about people and more curious about ideas.",
        "Marie Curie"
    },
    {
        "If I have seen further it is by standing on the shoulders of giants.",
        "Isaac Newton"
    },
    {
        "The important thing is not to stop questioning. Curiosity has its own reason for existing.",
        "Albert Einstein"
    },
    {
        "The scientist is not a person who gives the right answers, he's one who asks the right questions.",
        "Claude Levi-Strauss"
    },
    {
        "In questions of science, the authority of a thousand is not worth the humble reasoning of a single individual.",
        "Galileo Galilei"
    },
    {
        "Nothing in life is to be feared, it is only to be understood.",
        "Marie Curie"
    },
    {
        "Equipped with his five senses, man explores the universe around him and calls the adventure Science.",
        "Edwin Hubble"
    },
    {
        "The good thing about science is that it's true whether or not you believe in it.",
        "Neil deGrasse Tyson"
    },
    {
        "I have no special talent. I am only passionately curious.",
        "Albert Einstein"
    },
    {
        "What we know is a drop, what we don't know is an ocean.",
        "Isaac Newton"
    },
    {
        "The most beautiful thing we can experience is the mysterious.",
        "Albert Einstein"
    },
    {
        "We are just an advanced breed of monkeys on a minor planet of a very average star.",
        "Stephen Hawking"
    },
    {
        "To raise new questions, new possibilities, to regard old problems from a new angle, requires creative imagination.",
        "Albert Einstein"
    },
    {
        "Research is what I'm doing when I don't know what I'm doing.",
        "Wernher von Braun"
    },{
        "To succeed in your mission, you must have single-minded devotion to your goal.",
        "A.P.J. Abdul Kalam"
    },
};