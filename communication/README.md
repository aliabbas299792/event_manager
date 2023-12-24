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

## Adding more request types and response data types
To add a new type, there are 3 modifications needed:
1. Add a new enum request type representing the request in `communiation_types.hpp`
2. Add a new data type which would store the response data you want in `response_packs.hpp`
3. Add a new type trait specialisation for the enum value (i.e a new mapping from the enum to the data type)
4. Add `RespDataTypeMap<RequestType::your_new_request_enum>` to the variant at the bottom (before the monostate)