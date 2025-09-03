I'll modify the shield usage strategy to be much more conservative, limiting it only to survival-critical situations and preventing its use in the final 5 ticks.
Now I'll update the `judge` function to limit shield usage in survival-critical situations only:
Let me modify the shield usage in `enhancedFoodProcessByPriority` to make it more conservative:
Let me apply the same change to the second occurrence:
Now let me update the special target handling for chests and keys to be more conservative with shield usage:
Let me update the other shield usage occurrences in the same pattern:
I need to be more specific with my replacements:
Read file: src/snake.cpp
Let me apply specific replacements for each shield usage:
Let me try updating each instance separately:
Now let's update the second instance:
Finally, let's update the shield usage for safe zone emergencies:
Read file: src/snake.cpp
Let's update the safe zone shield usage:
Let's update the `enhancedFoodProcessByPriority` function to be more conservative with shield usage:
Read file: src/snake.cpp
Let's update the shield usage in `enhancedFoodProcessByPriority`:
Let's modify one more instance where shields are used, to ensure they're only used when absolutely necessary:
Let me try with the correct text:
Read file: src/snake.cpp
Let's update the shield decision logic in `enhancedFoodProcessByPriority`:
Let's review the changes we've made to restrict shield usage:

1. Completely rewrote the `shouldUseShield` function to only allow shields in true survival situations
2. Added checks to ensure shields are never used in the last 5 ticks
3. Removed shield usage for food collection except in truly deadly situations
4. Added extra checks for shield usage during safe zone shrinking to ensure it's only used when absolutely necessary
5. Updated all shield usage calls to check for remaining ticks
6. Added additional checks to verify that the situation is truly dangerous (e.g., checking that 3+ directions are illegal)

These changes should strictly limit shield usage to only "必死无疑" (certain death) situations, while avoiding shield usage in the last 5 ticks. The snake will be more conservative with its shields and only use them as a last resort for survival.
