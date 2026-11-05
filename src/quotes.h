#ifndef QUOTES_H
#define QUOTES_H
/* Original daily encouragements, stored locally; no network required. */
static const char *quotes[]={
    "Small steps, taken every day, can carry you farther than you imagine.",
    "You do not need to see the whole path to take the next step.",
    "Let progress be your measure today, even when it feels small.",
    "Make room for one thing today that your future self will thank you for.",
    "Begin where you are. Use what you have. Keep learning as you go.",
    "A difficult day can still hold a moment of courage.",
    "Give your attention to the next useful thing you can do.",
    "Rest is part of the journey. Return with a little more kindness for yourself.",
    "Your pace can be gentle and your direction can still be forward.",
    "One thoughtful choice today can open a new possibility tomorrow.",
    "Practice turns an uncertain beginning into a familiar strength.",
    "You can start again without starting from nothing. Experience comes with you.",
    "Choose curiosity when the answer is not yet clear.",
    "Make something a little better than you found it.",
    "A quiet effort still counts, even when no one sees it.",
    "Leave a little space in your day for something that makes you smile.",
    "You have time to take a breath and choose your next step.",
    "The work you return to is the work that grows.",
    "Be patient with the part of you that is still learning.",
    "A kind word can be a small beginning with a lasting effect.",
    "Focus on what you can shape, and give it your care.",
    "There is strength in asking for help and wisdom in accepting it.",
    "Today is another chance to practice the person you want to become.",
    "Keep a promise to yourself today, however small.",
    "Let a setback teach you something without defining you.",
    "Notice what is working. Build from there.",
    "You can be proud of an honest effort before you know the result.",
    "Give your best energy to what matters most to you.",
    "A little courage is enough to begin a meaningful change.",
    "Celebrate the distance you have come, then take one more step.",
    "There is possibility in an ordinary day when you meet it with intention.",
};
static int quote_index(Date d){
    return days_before(d.year,d.month,d.day)%(int)(sizeof(quotes)/sizeof(quotes[0]));
}
#endif
