#!/bin/bash

SESSION_NAME="ang"

if tmux has-session -t $SESSION_NAME 2> /dev/null; then
	tmux attach -t $SESSION_NAME
	exit
fi

# Open Vim
tmux new-session -d -s $SESSION_NAME -n dev
tmux send-keys -t $SESSION_NAME:dev "cd ${PWD}" C-m
tmux send-keys -t $SESSION_NAME:dev "vim" C-m

# Open up a tab for git
tmux new-window -n git -t $SESSION_NAME
tmux send-keys -t $SESSION_NAME:git "cd ${PWD}" C-m
tmux send-keys -t $SESSION_NAME:git "git hista" C-m

tmux select-window -t $SESSION_NAME:0

tmux attach -t $SESSION_NAME
