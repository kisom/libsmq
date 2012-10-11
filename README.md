# libsmq

This is a simple message queue modeled after Go's channels. It is released
under an ISC license. It uses pthreads for locking, so you will need a
working pthreads library to use this.


INSTALLING
----------
* Globally:
        ./config.sh && make install

* For the current user:
        PREFIX=${HOME} ./config.sh && make install

* To uninstall, simple substitute install with uninstall. Make sure the
same prefix you installed to is configured, otherwise it will not
uninstall properly.


EXAMPLE
-------
The included file `smq_test` shows an example of a single-threaded and
multi-threaded use case.


COMPATIBILITY
-------------
This runs on the BSDs. The Linux systems I tried this on took offense to my
used of the timedlocks, so those are #defined to a trylock.
