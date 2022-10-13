ADR 0010: Acknowledge stability and maintainability tasks
=========================================================

- Status: accepted
- Deciders: Jan-Lukas, Neil, Simon
- Date: 2021-10-12

Context
-------

Over the past couple of years Scipp development was repeatedly prioritizing "getting things to work" and "adding new features".
This was justified since we had to prove to stakeholders that we will be able to support all the required features.
While we have always made sure that there are unit tests for any added feature and that code is reviewed, we nevertheless feel that there is significant room to improve, in particular since Scipp is intended to grow into an essential building block with a long life span.

Independent of the above situation we also acknowledge that it is generally more satisfying to work on new features, fixing bugs, or improving performance.
If a developer implements such improvements they tend to get a "thumbs up" or "thanks" from the reviewer, but at the end of the month it may nevertheless feel less satisfying than having a number of new features added to the release notes.

The aim of this ADR is to foster a project culture where working on "non-functional" improvements is more satisfying and is acknowledged both internally and externally.

Decision
--------

Publicly acknowledge work and developers that contribute to long-term project maintainability.

- Add a new section to the release notes, named "Stability, Maintainability, and Testing".
- Add entries to this section for tasks that address such issues or improve the current situation.
  This includes improving unit testing, improving CI, improving documentation, and more.
- Reviewers of PRs should consider to add such entries (and push to the original PR branch) if not done by PR author.
  This is a clear statement "I think what you did deserves public acknowledgement".
- Consider adding names to all entries in the release notes (not just in the new section).

Consequences
------------

Positive:
~~~~~~~~~

- May lead to higher developer satisfaction and motivation to pick up non-functional development tasks.
- May aid in communicating to users that Scipp tries to take stability and maintainability seriously.

Negative:
~~~~~~~~~

- Minor extra work for adding notes and links to PRs to release notes.
