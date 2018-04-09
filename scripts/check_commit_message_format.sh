#!/bin/bash
# Copyright (c) 2018 Valve Corporation
# Copyright (c) 2018 LunarG, Inc.

# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
# Script to determine if source code in Pull Request is properly formatted.
# Exits with non 0 exit code if formatting is needed.

# Checks commit messages against project CONTRIBUTING.md document

RED='\033[0;31m'
GREEN='\033[0;32m'
NC='\033[0m' # No Color

# TRAVIS_COMMIT_RANGE contains range of commits for PR

# Get user-supplied commit message text for applicable commits
COMMIT_TEXT=$(git log ${TRAVIS_COMMIT_RANGE} --pretty=format:"XXXNEWLINEXXX"%n%B)

# Bail if there are none
if [ -z "${COMMIT_TEXT}" ]; then
  echo -e "${GREEN}No source code to check for formatting.${NC}"
  exit 0
fi

# Process commit messages
cnt=0
printf %s "$COMMIT_TEXT" | while IFS='' read -r line; do
  echo "Count = $cnt <Line> = $line"
  cnt=$((cnt+1))
  if [ "$line" = "XXXNEWLINEXXX" ]; then
    cnt=0
  fi
  length=${#line}
  if [ $cnt -eq 1 ]; then
    # Checking if subject exceeds 50 characters (but give some slack here)
    if [ $length -gt 54 ]; then
      echo "Your subject line exceeds 50 characters."
      exit 1
    fi
    i=$(($length-1))
    last_char=${line:$i:1}
    # Last character must not have a punctuation
    if [[ ! $last_char =~ [0-9a-zA-Z] ]]; then
      echo "Last character of the subject line must not have punctuation."
      exit 1
    fi
    # Checking if subject line doesn't start with 'module: '
    prefix=$(echo $line | cut -f1 -d " ")
    echo " prefix for this line was $prefix"
    if [ "${prefix::-1}" != ":" ]; then
      echo "Your subject line must start with a single word specifying the functional area of the change, followed by a colon and space."
      echo "I.e., 'layers: Subject line here'"
      exit 1
    fi
  elif [ $cnt -eq 2 ]; then
    # Subject must be followed by a blank line
    if [ $length -ne 0 ]; then
      echo "Your subject line follows a non-empty line. Subject lines should always be followed by a blank line."
      exit 1
    fi
  else
    # Any line in body must not exceed 72 characters (but give some slack)
    if [ $length -gt 76 ]; then
      echo "The line \"$line\" exceeds 72 characters."
      exit 1
    fi
  fi
done

echo -e "${GREEN}All commit messages in PR properly formatted.${NC}"
exit 0
