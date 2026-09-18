#ifndef WEB_CSS_H
#define WEB_CSS_H

#include "../WList.h"
#include "Arduino.h"

const static char CSS_GENERAL_ID[] PROGMEM = "*";
const static char CSS_GENERAL_STYLE[] PROGMEM = R"=====(
--db: #1d1d22;
--lb: #2e2e32;
--df: #98a2b0;
--lf: #eef1f5;
--bb: #16a085;
--bh: #18b193;
font-family:system-ui,Roboto,sans-serif;
margin:0;
font-size:1rem;
gap:.5rem;
)=====";

const static char CSS_BODY_ID[] PROGMEM = "body";
const static char CSS_BODY_STYLE[] PROGMEM = R"=====(
background:var(--db);
color:var(--lf);
)=====";

//const static char CSS_H2_ID[] = WC_H2;
const static char CSS_H2_STYLE[] PROGMEM = R"=====(
margin:0;
font-size:1.2rem;
)=====";

const static char CSS_BAR_ID[] PROGMEM = ".bar";
const static char CSS_BAR_STYLE[] PROGMEM = R"=====(
position:sticky;
top:0;
padding:.5rem.5rem;
z-index:3;
display:flex; 
align-items:center; 
background:var(--lb);
)=====";

//CSS_BUTTON_ID = WC_BUTTON
const static char CSS_BUTTON_STYLE[] PROGMEM = R"=====(
padding:.5rem.8rem;
border-radius:.4rem;
border:none;
background:var(--bb);
color:var(--lf);
)=====";

const static char CSS_BUTTON_HOVER_ID[] PROGMEM = "button:hover";
const static char CSS_BUTTON_HOVER_STYLE[] PROGMEM = "background:var(--bh);";
const static char CSS_BUTTON_ON_ID[] PROGMEM = "button.on";
const static char CSS_BUTTON_ON_STYLE[] PROGMEM = "background:var(--db);";

const static char CSS_BUTTON_ICON_CLASS[] PROGMEM = "icon";
const static char CSS_BUTTON_ICON_ID[] PROGMEM = ".icon";
const static char CSS_BUTTON_ICON_STYLE[] PROGMEM = R"=====(
background:transparent;
color:var(--bb);
)=====";

const static char CSS_BUTTON_ICON_HOVER_ID[] PROGMEM = ".icon:hover";
const static char CSS_BUTTON_ICON_HOVER_STYLE[] PROGMEM = "background:var(--db);";

const static char CSS_BURGER_BUTTON_CLASS[] PROGMEM = "icon burger";
const static char CSS_BURGER_ID[] PROGMEM = ".icon.burger";
//WC_DISPLAY_NONE

const static char CSS_SWITCH_ID[] = ".sw";
const static char CSS_SWITCH_STYLE[] PROGMEM = R"=====(
position: relative;
border: 0;
flex: 0 0 auto;
width: 2.9rem;
height: 1.6rem;
margin: 0;
padding: 0;
border-radius: 1rem;
background:var(--db);
transition: background .18s;
)=====";
const static char CSS_SWITCH_ID_CHECKED[] = ".sw[aria-checked=true]";
const static char CSS_SWITCH_STYLE_CHECKED[] PROGMEM = R"=====(
background:var(--bb);
)=====";
const static char CSS_SWITCH_ID_I[] = ".sw i";
const static char CSS_SWITCH_STYLE_I[] PROGMEM = R"=====(
position: absolute;
top:.2rem;
left:.2rem;
width:1.2rem;
height:1.2rem;
border-radius:50%;
background:var(--lb);
transition:transform.2s;
)=====";
const static char CSS_SWITCH_ID_CHECKED_I[] = ".sw[aria-checked=true] i";
const static char CSS_SWITCH_STYLE_CHECKED_I[] PROGMEM = R"=====(
transform: translateX(1.3rem);
)=====";

//a drop down list in a card: the ground, the radius and the padding the other
//controls carry, and no more width than its longest option asks for
const static char CSS_SELECT_ID[] = ".sel";
const static char CSS_SELECT_STYLE[] PROGMEM = R"=====(
flex:0 0 auto;
padding:.4rem.6rem;
border:1px solid var(--lb);
border-radius:.4rem;
background:var(--db);
color:var(--lf);
font:inherit;
)=====";

const static char CSS_SEGMENT_ID[] = ".seg";
const static char CSS_SEGMENT_STYLE[] PROGMEM = R"=====(
display:flex;
border-radius:.4rem;
gap:.2rem;
padding:.2rem;
background:var(--db);
)=====";

const static char CSS_SEGMENT_BUTTON_ID[] = ".seg button";
const static char CSS_SEGMENT_BUTTON_STYLE[] PROGMEM = R"=====(
margin:0;
padding:.3rem.7rem;
background:0 0;
color:var(--df);
font-size:.9rem;
)=====";

const static char CSS_SEGMENT_BUTTON_PRESSED_ID[] = ".seg button[aria-pressed=true]";
const static char CSS_SEGMENT_BUTTON_PRESSED_STYLE[] PROGMEM = R"=====(
background:var(--bb);
color:var(--lf);
)=====";


const static char CSS_SHELL_ID[] PROGMEM = ".shell";
const static char CSS_SHELL_STYLE[] PROGMEM = R"=====(
display:flex;
align-items:flex-start;
width:100%;
max-width:58rem;
margin:0 auto;
padding:.5rem .5rem;
)=====";

const static char CSS_SIDE_ID[] PROGMEM = ".side";
const static char CSS_SIDE_STYLE[] PROGMEM = R"=====(
position:sticky;
flex:0 0 12rem;
padding:1rem.5rem;
background:var(--lb);
border-radius:.7rem;
)=====";

const static char CSS_SIDE_BUTTON_ID[] PROGMEM = ".side button";
const static char CSS_SIDE_BUTTON_STYLE[] PROGMEM = "width:100%;";

const static char CSS_MAIN_CLASS[] PROGMEM = "main";
const static char CSS_MAIN_ID[] PROGMEM = "main";
const static char CSS_MAIN_STYLE[] PROGMEM = "flex:1;";

const static char CSS_CARD_CLASS[] PROGMEM = "card";
const static char CSS_CARD_ID[] PROGMEM = ".card";
const static char CSS_CARD_STYLE[] PROGMEM = R"=====(
margin-bottom:.5rem;
background:var(--lb);
border-radius:.7rem;
overflow:hidden;
)=====";  

const static char CSS_CARD_H3_ID[] PROGMEM = ".card > h3";
const static char CSS_CARD_H3_STYLE[] PROGMEM = R"=====(
display:flex;
align-items:baseline;
gap:.6rem;
margin:0;
padding:.8rem 1rem;
border-bottom:1px solid var(--db);
font-weight:600;
background:var(--lb);
)=====";  

//a row of small buttons at the right end of the title of a card
const static char CSS_CARD_TOOLS_CLASS[] PROGMEM = "tools";
const static char CSS_CARD_TOOLS_ID[] PROGMEM = ".card > h3 > .tools";
const static char CSS_CARD_TOOLS_STYLE[] PROGMEM = R"=====(
display:flex;
align-items:center;
gap:.2rem;
margin-left:auto;
align-self:center;
)=====";

//the head lines up its parts by their baseline, a square button has none that
//fits - so it is sized here and centered by the rule above
const static char CSS_CARD_TOOLS_BUTTON_ID[] PROGMEM = ".card > h3 > .tools button";
const static char CSS_CARD_TOOLS_BUTTON_STYLE[] PROGMEM = R"=====(
display:flex;
align-items:center;
justify-content:center;
width:1.7rem;
height:1.7rem;
padding:0;
font-size:1.2rem;
line-height:1;
)=====";

//const static char CSS_CARD_CLASS[] PROGMEM = "thing";
const static char CSS_CARD_ROW_CLASS[] PROGMEM = "crow";
const static char CSS_CARD_ROW_ID[] PROGMEM = ".crow";
const static char CSS_CARD_ROW_STYLE[] PROGMEM = R"=====(
display:flex;
align-items:center;
gap:1rem;
padding:.6rem 1rem;
border-bottom:1px solid var(--db);
background:var(--lb);
)=====";  

const static char CSS_CARD_ROW_READ_ONLY_CLASS[] PROGMEM = "crow ro";
const static char CSS_CARD_ROW_READ_ONLY_ID[] PROGMEM = ".crow.ro";
const static char CSS_CARD_ROW_READ_ONLY_STYLE[] PROGMEM = "color:var(--df);";

const static char CSS_CARD_LABEL_CLASS[] PROGMEM = "lbl";
const static char CSS_CARD_LABEL_ID[] PROGMEM = ".lbl";
const static char CSS_CARD_LABEL_STYLE[] PROGMEM = R"=====(
flex:0 0 10rem;
margin:0;
font-size:.9rem;
)=====";  

const static char CSS_CARD_CTL_CLASS[] PROGMEM = "ctl";
const static char CSS_CARD_CTL_ID[] PROGMEM = ".ctl";
const static char CSS_CARD_CTL_STYLE[] PROGMEM = R"=====(
display:flex;
flex:1;
min-width:0;
align-items:center;
justify-content:flex-end;
gap:.7rem;
)=====";  


const static char CSS_INPUT_ID[] PROGMEM = "input[type=text],input[type=password],input[type=number],select,textarea";
const static char CSS_INPUT_STYLE[] PROGMEM = R"=====(
width: 100%;
padding:.4rem.6rem;
border:1px solid var(--lb);
border-radius:.4rem;
background:var(--db);
color:var(--lf);
font:inherit;
)=====";  

const static char CSS_MEDIA_MOBILE_ID[] PROGMEM = "@media(max-width:44rem)";
const static char CSS_MEDIA_MOBILE_STYLE[] PROGMEM = R"=====(
  .lbl {
  flex-basis:7rem;
  }
  .icon.burger {
  display:block;
  }
  .side {
  position:fixed;
  top:3rem;
  bottom:0;
  left:0;
  z-index:2;
  width:12rem;
  background:var(--lb);
  transform:translateX(-100%);
  transition:transform .2s;
  border-right:1px solid val(--db);
  }
  body.nav .side {
  transform:none;
  }
  main {
  padding:1rem;
  }
)=====";

const static char CSS_MEDIA_LIGHT_ID[] PROGMEM = "@media(prefers-color-scheme:light)";
const static char CSS_MEDIA_LIGHT_STYLE[] PROGMEM = R"=====(
  * {
  --db: #ebebed;
  --lb: #ffffff;
  --df: #5a6472;
  --lf: #1d1d22;
  }
)=====";

#endif