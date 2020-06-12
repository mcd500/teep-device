These are just enough missing pieces to allow
libwebsockets.h to compile in core context.

The networking-related pieces will be broken
by this, but networking is for TAs.

Only Pseudo-TAs should get their includes from
here.  TAs should get them from the usual
optee out export- "dev" dir.
