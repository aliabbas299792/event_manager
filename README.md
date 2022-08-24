# Event Manager
Simple liburing based library which uses callbacks for I/O events.
It should now be possible to shutdown the server at any time without there being issue with memory/resource leaks.

In the callbacks check if `operation_metadata.op_res_num == -ECANCELED` to check if the operation was cancelled, and for any ongoing requests during the shutdown of the event manager, you can check `ev->get_living_state() > event_manager::living_state::LIVING` to know if the callback was triggered during the shutdown stage, and then you can clean up resources as appropriate.