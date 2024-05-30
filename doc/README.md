# tmedia/doc

For more developer-facing documentation, such as design goals, software
structure, and building specifics, the [doc/dev](./dev/README.md) directory
contains much of such relevant information.

The structure of this directory isn't really concrete, if something is both
developer facing and user facing, it should be in same directory level as this 
document and block out developer documentation with "\>" blocks with a heading
**DEV:** at the top, like:

> **DEV:**
>
> A developer comment, users should not need to worry about reading
> the content of this block.

Note that these documents are written in a way where it's encouraged that
you only read what you actually care about! Most things work as you'd expect,
and the help text given by ```tmedia --help``` should be more than enough
to get started. These documents are moreso a concrete way to confirm tmedia's
behavior and possibly identify if certain behaviors are bugs or "features".

> **CLARIFICATION:**
>
> Of course, if a "feature" is unintuitive, that is itself a bug!