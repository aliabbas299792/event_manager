This is a simple class with some datatypes to facilitate passing data from the event loop to the coroutines safely.

## File overview

### communication_types.hpp
This defines the different request types there are, and then effectively constructs a map below from those enumerated request types to the data types which would carry back the response data.

### response_packs.hpp
These are the datatypes used to respond to requests from the coroutines, the ones the enumerated request types mentioned above are mapped to.
At the bottom is the variant combining them all (`monostate` is at the end used to indicate an empty variant).

### communication_channel.hpp
This is a simple wrapper around `std::variant`, for getting data it resets the value held, and returns an optional - the optional may not be needed but I've kept it just in case.

### communication_tests.cpp
A bunch of simple sanity testing unit tests.
