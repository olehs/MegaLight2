#ifndef ENUMS_H
#define ENUMS_H

namespace ButtonEvent {
enum Type {
    StateChanged,
    Pressed,
    Released,
    Repeat,
    Hold,
    LongClick,
    Click,
    DoubleClick,
    EventsCount // must be last
};
}

namespace InputPullup {
enum PullupType {
    PullDown,
    IntPullup,
    ExtPullup
};
}

namespace ButtonState {
enum State {
    Any = 0,
    Up = 1,
    Down = 2,
    HoldState = 4
};
}

namespace OutputAction {
enum Action {
    Unassigned,
    NoAction,
    On,
    Off,
    Toggle,
    Value,
    IncValue
};
}

namespace OutputStateSave {
enum Save {
    None,
    State,
    Value,
    StateAndValue
};
}

#endif // ENUMS_H
