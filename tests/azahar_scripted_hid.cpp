#include "../tools/azahar_scripted_hid.h"
#include <cassert>
#include <optional>

int main(int argc, char** argv) {
    assert(argc == 2);
    for (const char* bad : {"", "1 0 0 0\n", "0 -1 0 0\n", "0 4096 0 0\n",
                           "0 0 155 0\n", "0 0 0 -155\n", "0 0 0 0 extra\n",
                           "0 0 0 0\n0 1 0 0\n", "-1 0 0 0\n", "0 0 0\n",
                           "0 0 0 0\n10000001 0 0 0\n", "0 8192 0 0\n",
                           "0 0 0 0 1\n", "0 0 0 0 157 0\n", "0 0 0 0 0 -157\n", "0 0 0 0 0 0 320 0 1\n", "0 0 0 0 0 0 0 240 1\n", "0 0 0 0 0 0 0 0 2\n"}) {
        bool rejected = false;
        try { std::istringstream input(bad); (void)FoxhollowHid::Parse(input); }
        catch (const std::runtime_error&) { rejected = true; }
        assert(rejected);
    }
    std::string many;
    for (unsigned i = 0; i < 4097; ++i) many += std::to_string(i) + " 0 0 0\n";
    bool rejected = false;
    try { std::istringstream input(many); (void)FoxhollowHid::Parse(input); }
    catch (const std::runtime_error&) { rejected = true; }
    assert(rejected);
    unsetenv("FH_HID_SCRIPT");
    FoxhollowHid::Replay replay;
    struct Pad { unsigned hex = 123; } pad;
    short x = 17, y = 18;
    unsigned reads = 0;
    std::optional<unsigned> frame;
    auto read = [&](unsigned address) { assert(address == 0x100000); ++reads; return frame; };
    replay.Apply(read, pad, x, y);
    assert(pad.hex == 123 && x == 17 && y == 18 && reads == 0);
    std::istringstream script("0 0 0 0\n22 8 154 -154\n24 0 0 0\n200 4 0 0\n");
    replay.events = FoxhollowHid::Parse(script);
    replay.address = 0x100000;
    replay.trace = std::fopen(argv[1], "wx");
    assert(replay.trace);
    replay.Apply(read, pad, x, y);
    assert(!frame && pad.hex == 0 && x == 0 && y == 0);
    for (unsigned i = 0; i <= 201; ++i) {
        frame = i;
        replay.Apply(read, pad, x, y);
        const bool start = i >= 22 && i < 24;
        assert(pad.hex == (start ? 8u : i >= 200 ? 4u : 0u));
        assert(x == (start ? 154 : 0) && y == (start ? -154 : 0));
        assert(*frame == i);
    }
    std::istringstream extra_script("0 0 0 0\n202 49153 7 -8 156 -156 42 199 1\n204 4 0 0\n");
    replay.events = FoxhollowHid::Parse(extra_script);
    replay.index = 0;
    struct Touch {unsigned x=0,y=0;struct {unsigned value=0;void Assign(unsigned v){value=v;}} valid;} touch;
    frame = 202;
    replay.Apply(read, pad, x, y);
    assert(pad.hex == 1 && x == 7 && y == -8);
    replay.ApplyTouch(touch);assert(touch.x==42&&touch.y==199&&touch.valid.value==1);
    replay.Apply(read, pad, x, y, true);
    assert(pad.hex == 49152 && x == 156 && y == -156);
    frame = 204;
    replay.Apply(read, pad, x, y, true);
    assert(pad.hex == 0 && x == 0 && y == 0);
    replay.ApplyTouch(touch);assert(touch.valid.value==0);
    frame = 1;
    rejected = false;
    try { replay.Apply(read, pad, x, y); }
    catch (const std::runtime_error&) { rejected = true; }
    assert(rejected);
    // Separate fixture trace: startup progresses, then freezes while game
    // retraces begin. No input callback writes either guest observation.
    FoxhollowHid::Replay handoff;
    handoff.address=0x100000;handoff.add_address=0x100004;
    std::istringstream handoff_script("0 0 0 0\n120 1 0 0\n130 0 0 0\n180 4 0 0\n");
    handoff.events=FoxhollowHid::Parse(handoff_script);
    handoff.trace=std::tmpfile();assert(handoff.trace);
    unsigned menu=0,game=0;bool mapped=true;
    auto pair_read=[&](unsigned at)->std::optional<unsigned>{
        assert(at==0x100000||at==0x100004);
        if(at==0x100004&&!mapped)return std::nullopt;
        return at==0x100000?menu:game;
    };
    for(unsigned total=0;total<=200;total++){
        menu=total<120?total:120;game=total-menu;
        handoff.Apply(pair_read,pad,x,y);
        assert(pad.hex==(total>=180?4u:total>=120&&total<130?1u:0u));
        assert(menu+game==total);
    }
    mapped=false;handoff.Apply(pair_read,pad,x,y);assert(!pad.hex);mapped=true;
    menu=119;game=100;rejected=false;
    try{handoff.Apply(pair_read,pad,x,y);}catch(const std::runtime_error&){rejected=true;}
    assert(rejected);menu=120;game=UINT32_MAX;rejected=false;
    try{handoff.Apply(pair_read,pad,x,y);}catch(const std::runtime_error&){rejected=true;}
    assert(rejected);
    const std::string fixture_path=std::string(argv[1])+".script";
    {std::ofstream fixture(fixture_path);fixture<<"0 0 0 0\n";}
    setenv("FH_HID_SCRIPT",fixture_path.c_str(),1);
    setenv("FH_HID_ADDRESS","0x100000",1);
    const std::string pair_trace=std::string(argv[1])+".pair";
    setenv("FH_HID_TRACE",pair_trace.c_str(),1);
    for(const char*bad:{"","nope","0x100000","0x100001","0xffffc","0x10000000","-1"}){
        setenv("FH_HID_COUNTER_ADD_ADDRESS",bad,1);rejected=false;
        try{FoxhollowHid::Replay invalid;}
        catch(const std::runtime_error&e){rejected=std::string(e.what())=="Invalid read-only FH_HID_COUNTER_ADD_ADDRESS";}
        assert(rejected);
    }
    setenv("FH_HID_COUNTER_ADD_ADDRESS","0x100004",1);
    {FoxhollowHid::Replay valid;assert(valid.address==0x100000&&valid.add_address==0x100004);}
    unsetenv("FH_HID_SCRIPT");unsetenv("FH_HID_COUNTER_ADD_ADDRESS");
    // Adaptive input must work after idle, accept complete appended commands
    // only, preserve accepted events, and reject stale/invalid commands atomically.
    const auto live_path = fixture_path+".live";
    {std::ofstream file(live_path);}
    FoxhollowHid::Replay adaptive;
    adaptive.address=0x100000;
    std::istringstream initial("0 0 0 0\n");
    adaptive.events=FoxhollowHid::Parse(initial);
    adaptive.trace=std::tmpfile();assert(adaptive.trace);
    adaptive.live.open(live_path,std::ios::binary);
    auto append=[&](const std::string& s){std::ofstream file(live_path,std::ios::app|std::ios::binary);file<<s;assert(file);};
    adaptive.PollLive(0,true);assert(adaptive.events.size()==1);
    append("20 2 0");adaptive.PollLive(1,true);assert(adaptive.events.size()==1);
    append(" 0\n30 0 0 0\n");adaptive.PollLive(2,true);assert(adaptive.events.size()==3);
    for(unsigned i=2;i<40;i++){
        frame=i;adaptive.Apply(read,pad,x,y);assert(pad.hex==(i>=20&&i<30?2u:0u));
    }
    append("60 1 0 0 156 -156 42 199 1\n70 0 0 0\n");
    adaptive.PollLive(40,true);assert(adaptive.events.size()==5);
    frame=60;adaptive.Apply(read,pad,x,y);adaptive.ApplyTouch(touch);
    assert(pad.hex==1&&touch.valid.value==1&&touch.x==42&&touch.y==199);
    frame=70;adaptive.Apply(read,pad,x,y);adaptive.ApplyTouch(touch);assert(!pad.hex&&!touch.valid.value);
    for(const std::string& bad: {std::string("71 1 0 0\n"),std::string("70 1 0 0\n"),std::string("80 4096 0 0\n"),
                              std::string("80 2 0 0\n79 0 0 0\n"),std::string("\n"),std::string(129,'1')}){
        FoxhollowHid::Replay reject;
        reject.events=adaptive.events;
        const auto bad_path=live_path+".bad";
        {std::ofstream file(bad_path);file<<bad;}
        reject.live.open(bad_path,std::ios::binary);rejected=false;
        try{reject.PollLive(71,true);}catch(const std::runtime_error&){rejected=true;}
        assert(rejected&&reject.events.size()==5);
    }
    {std::ofstream file(live_path);file<<"x";}
    rejected=false;try{adaptive.PollLive(80,true);}catch(const std::runtime_error&){rejected=true;}
    assert(rejected&&adaptive.events.size()==5);
    std::puts("Adaptive HID: empty/partial append, two button pulses, IR/touch fields, late/reversed/invalid input and truncation rejected");
    std::puts("HID script: malformed bounds, inert default, unmapped wait, 202 input states; 201 additive handoff states, address validation, overflow and constituent reversal passed");
}
