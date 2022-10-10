# decoder



## Getting started

To make it easy for you to get started with GitLab, here's a list of recommended next steps.

Already a pro? Just edit this README.md and make it your own. Want to make it easy? [Use the template at the bottom](#editing-this-readme)!

## Add your files

- [ ] [Create](https://docs.gitlab.com/ee/user/project/repository/web_editor.html#create-a-file) or [upload](https://docs.gitlab.com/ee/user/project/repository/web_editor.html#upload-a-file) files
- [ ] [Add files using the command line](https://docs.gitlab.com/ee/gitlab-basics/add-file.html#add-a-file-using-the-command-line) or push an existing Git repository with the following command:

```
cd existing_repo
git remote add origin https://gitlab.com/qbau/software-and-apps/decoder.git
git branch -M main
git push -uf origin main
```

## Integrate with your tools

- [ ] [Set up project integrations](https://gitlab.com/qbau/software-and-apps/decoder/-/settings/integrations)

## Collaborate with your team

- [ ] [Invite team members and collaborators](https://docs.gitlab.com/ee/user/project/members/)
- [ ] [Create a new merge request](https://docs.gitlab.com/ee/user/project/merge_requests/creating_merge_requests.html)
- [ ] [Automatically close issues from merge requests](https://docs.gitlab.com/ee/user/project/issues/managing_issues.html#closing-issues-automatically)
- [ ] [Enable merge request approvals](https://docs.gitlab.com/ee/user/project/merge_requests/approvals/)
- [ ] [Automatically merge when pipeline succeeds](https://docs.gitlab.com/ee/user/project/merge_requests/merge_when_pipeline_succeeds.html)

## Test and Deploy

Use the built-in continuous integration in GitLab.

- [ ] [Get started with GitLab CI/CD](https://docs.gitlab.com/ee/ci/quick_start/index.html)
- [ ] [Analyze your code for known vulnerabilities with Static Application Security Testing(SAST)](https://docs.gitlab.com/ee/user/application_security/sast/)
- [ ] [Deploy to Kubernetes, Amazon EC2, or Amazon ECS using Auto Deploy](https://docs.gitlab.com/ee/topics/autodevops/requirements.html)
- [ ] [Use pull-based deployments for improved Kubernetes management](https://docs.gitlab.com/ee/user/clusters/agent/)
- [ ] [Set up protected environments](https://docs.gitlab.com/ee/ci/environments/protected_environments.html)

***

# Editing this README

When you're ready to make this README your own, just edit this file and use the handy template below (or feel free to structure it however you want - this is just a starting point!). Thank you to [makeareadme.com](https://www.makeareadme.com/) for this template.

## Suggestions for a good README
Every project is different, so consider which of these sections apply to yours. The sections used in the template are suggestions for most open source projects. Also keep in mind that while a README can be too long and detailed, too long is better than too short. If you think your README is too long, consider utilizing another form of documentation rather than cutting out information.


## Description
These are control scripts for the quantum decoder and its variants. The function of the full decoder is to take a probability table indicating the probability of each symbol in an alphabet at each of a set of timesteps and find the most likely class of strings. The probability table is provided and comes from a classical convolutional neural network as part of a compound application, where our focus has been on a speech-to-text application. The classes of strings are derived by applying a process to the strings and declaring them equivalent if they have the same output. Concretely, the current process has two steps:
1. Contract all repetitions within the string down to one symbol,
2. Remove all _null_ characters.

To find the probabilities of each string class, which we call a _beam_, the decoder finds the probabilities of each string in the beam and adds them. It then uses repeated Grover searches to magnify the highest probability beam as high as possible so that it is returned upton quantum measurement

### Simplified decoder
In order to reduce the scaling of the gate depth and to have a relevant application demonstrable to clients we have also put together a simplified version of the decoder which does not identify the beams but simply encodes the strings with probability amplitudes representative of the input probability table. The probability of any given string being returned upon measurement matches that expected from the probability table. Similarly for the probability of the returned string belonging to a given beam. The beam to which the returned string belongs is determined classically post-measurement. This simplified approach does not attempt to return the highest probability string or beam with certainty, _i.e._ there is no amplitude amplification of the highest probability string/beam.

## Tests
Test information may be added here

## Project status
This is a branch of the SDK-core
