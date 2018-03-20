# Contributing to The ioquake3 Project

The following is a set of guidelines for contributing to ioquake3 which is hosted in the [The ioquake Group](https://github.com/ioquake) on Github. These are mostly guidelines, not rules. Use your best judgment, and feel free to propose changes to this document in a pull request.

## Table Of Contents

[I don't want to read this whole thing, I just have a question!!!](#i-dont-want-to-read-this-whole-thing-i-just-have-a-question)

[How Can I Contribute?](#how-can-i-contribute)
  * [Reporting Bugs](#reporting-bugs)
  * [Suggesting Enhancements](#suggesting-enhancements)
  * [Your First Code Contribution](#your-first-code-contribution)
  * [Pull Requests](#pull-requests)
[Additional Notes](#additional-notes)
  * [Issue and Pull Request Labels](#issue-and-pull-request-labels)

## I don't want to read this whole thing I just have a question!!!

> **Note:** Please don't file an issue to ask a question. You'll get faster results by using the resources below.

We have an official message board and a wiki where the community provides helpful advice if you have questions.

* [Discourse, the official ioquake message board](https://discourse.ioquake.org)
* [ioquake3 wiki](http://wiki.ioquake3.org/Main_Page)

If you'd prefer to chat live with other users, sysadmins, and developers, we have IRC and Discord, with a bridge between the two:
* IRC: We're on Freenode (irc.freenode.net) in #ioquake3
* Or you can join our Discord [via this link](https://discord.gg/fPaGNuy)
With either of these services it might take a while before someone responds to your question, if you can't wait, [use the forums](https://discourse.ioquake.org)

## How Can I Contribute?

### Reporting Bugs

This section guides you through submitting a bug report for ioquake3. Following these guidelines helps maintainers and the community understand your report :pencil:, reproduce the behavior :computer: :computer:, and find related reports :mag_right:.

Before creating bug reports, please check [this list](#before-submitting-a-bug-report) as you might find out that you don't need to create one. When you are creating a bug report, please [include as many details as possible](#how-do-i-submit-a-good-bug-report). Fill out [the required template](ISSUE_TEMPLATE.md), the information it asks for helps us resolve issues faster.

> **Note:** If you find a **Closed** issue that seems like it is the same thing that you're experiencing, open a new issue and include a link to the original issue in the body of your new one.

#### Before Submitting A Bug Report

* **Check the latest version** The version in our installers is from 2009! Check if you can reproduce the problem [in the latest test build of ioquake3](http://ioquake3.org/get-it/test-builds/).
* **Search the [wiki](http://wiki.ioquake3.org/) and the [forum](https://discourse.ioquake.org)** for your questions and problems.
* **Perform a [cursory search](https://github.com/search?q=+is%3Aissue+user%3Aioquake)** to see if the problem has already been reported. If it has **and the issue is still open**, add a comment to the existing issue instead of opening a new one.

#### How Do I Submit A (Good) Bug Report?

Bugs are tracked as [GitHub issues](https://guides.github.com/features/issues/). Create an issue and provide the following information by filling in [the template](ISSUE_TEMPLATE.md).

Explain the problem and include additional details to help maintainers reproduce the problem:

* **Use a clear and descriptive title** for the issue to identify the problem.
* **Describe the exact steps which reproduce the problem** in as many details as possible. For example, start by explaining how you started ioquake3, e.g. which command exactly you used in the terminal, or how you started ioquake3 with a shortcut. When listing steps, **don't just say what you did, but explain how you did it**. For example, if you started a new map, explain if you used the menu, or a keyboard macro, or a / command in the ioquake3 console, and if so which one?
* **Provide specific examples to demonstrate the steps**. Include links to files or GitHub projects, or copy/pasteable snippets, which you use in those examples. If you're providing snippets in the issue, use [Markdown code blocks](https://help.github.com/articles/markdown-basics/#multiple-lines).
* **Describe the behavior you observed after following the steps** and point out what exactly is the problem with that behavior.
* **Explain which behavior you expected to see instead and why.**
* **Include screenshots** which show you following the described steps and clearly demonstrate the problem.
* **If you're reporting that ioquake3 crashed**, include a crash report with a stack trace from the operating system. Include the crash report in the issue in a [code block](https://help.github.com/articles/markdown-basics/#multiple-lines), a [file attachment](https://help.github.com/articles/file-attachments-on-issues-and-pull-requests/), or put it in a [gist](https://gist.github.com/) and provide link to that gist.
* **If the problem wasn't triggered by a specific action**, describe what you were doing before the problem happened and share more information using the guidelines below.

Provide more context by answering these questions:

* **Did the problem start happening recently** (e.g. after updating to a new version of ioquake3) or was this always a problem?
* If the problem started happening recently, **can you reproduce the problem in an older version of ioquake3?** What's the most recent version in which the problem doesn't happen?
* **Can you reliably reproduce the issue?** If not, provide details about how often the problem happens and under which conditions it normally happens.
* If the problem is related to working with third party pk3s and mods (e.g. opening and editing files), **does the problem happen with baseq3?** We need to be able to reproduce the issue with baseq3, usually.

Include details about your configuration and environment:

* **Which version of ioquake3 are you using?**
* **What's the name and version of the OS you're using?**
* **What graphics card or chipset is in your computer?**

### Helping others
People have questions, they ask them on our [forums](https://discourse.ioquake.org/), [Facebook](https://www.facebook.com/ioquake3/), [Live Chat](http://wiki.ioquake3.org/Live_Chat), and on other services and websites. Answer them if you can, and add them to the [Players Guide](http://wiki.ioquake3.org/Players_Guide) on the wiki or [Sys Admin Guide](http://wiki.ioquake3.org/Sys_Admin_Guide) if the same questions keep coming up. If the answers are already in the one of our guides, copy and paste the answer and then link the people asking questions to those pages if they have more questions.

### Donate
Money buys servers, hosting, and time. We can [accept donations](http://ioquake3.org/donate/) although we are not yet a 501(c)(3) nonprofit.

### Social Media
Throw ioquake3 a [Like on Facebook](https://www.facebook.com/ioquake3/) or [follow ioquake3 on Twitter](https://twitter.com/ioquake3). Let other Quake 3 players know about ioquake3, some people are still playing with id's client!

### Suggesting Enhancements

This section guides you through submitting an enhancement suggestion for ioquake3, including completely new features and minor improvements to existing functionality. Following these guidelines helps maintainers and the community understand your suggestion :pencil: and find related suggestions :mag_right:.

Before creating enhancement suggestions, please check [this list](#before-submitting-an-enhancement-suggestion) as you might find out that you don't need to create one. When you are creating an enhancement suggestion, please [include as many details as possible](#how-do-i-submit-a-good-enhancement-suggestion). Fill in [the template](ISSUE_TEMPLATE.md), including the steps that you imagine you would take if the feature you're requesting existed.

#### Before Submitting An Enhancement Suggestion

* **Check the latest test build** you might discover that the enhancement is already available.
* **Perform a [cursory search](https://github.com/search?q=+is%3Aissue+user%3Aioquake)** to see if the enhancement has already been suggested. If it has, add a comment to the existing issue instead of opening a new one.

#### How Do I Submit A (Good) Enhancement Suggestion?

Enhancement suggestions are tracked as [GitHub issues](https://guides.github.com/features/issues/). Create an issue on the ioquake3 repository and provide the following information:

* **Use a clear and descriptive title** for the issue to identify the suggestion.
* **Provide a step-by-step description of the suggested enhancement** in as many details as possible.
* **Provide specific examples to demonstrate the steps**. Include copy/pasteable snippets which you use in those examples, as [Markdown code blocks](https://help.github.com/articles/markdown-basics/#multiple-lines).
* **Describe the current behavior** and **explain which behavior you expected to see instead** and why.
* **Include screenshots** which help you demonstrate the steps or point out the part of ioquake3 which the suggestion is related to.
* **Explain why this enhancement would be useful** to most ioquake3 users.
* **Specify which version of ioquake3 you're using.** 
* **Specify the name and version of the OS you're using.**

### Your First Code Contribution

Unsure where to begin contributing to ioquake3?

If you want to read about using ioquake3, the [ioquake3 wiki](http://wiki.ioquake3.org) is free and available online.

#### Local development

ioquake3 can be developed locally. For instructions on how to do this, see [the instructions on our wiki for building it on your computer](http://wiki.ioquake3.org/Building_ioquake3).

### Pull Requests

* Fill in [the required template](PULL_REQUEST_TEMPLATE.md)
* Do not include issue numbers in the PR title.
