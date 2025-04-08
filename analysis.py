import numpy as np
import matplotlib.pyplot as plt
import pandas as pd

df = pd.read_csv('./data.csv')

def moving_average_smoother(data: np.ndarray, window_length: int) -> np.ndarray:
    output = np.zeros_like(data)

    for i in range(len(output)):
        average = 0
        if i < int(window_length / 2):
            for j in range(window_length):
                average += data[i + j]
            average /= window_length
            output[i] = average
        elif i > len(data) - int(window_length / 2):
            for j in range(window_length):
                average += data[i - j]
            average /= window_length
            output[i] = average
        else:
            for j in range(int(window_length / 2)):
                average += (data[i - j] + data[i + j])
            average /= window_length
            output[i] = average
    return output

f = plt.figure(figsize=(8, 4));
ax = f.gca()

#ax.plot(df['Time'], df['Average Tax Rate'], color='g', label="Average Tax Rate")
#ax.plot(df['Time'], df['Average Segmentation Rate'], color='k', alpha=0.3, label="Average Segmentation Rate")
ax.plot(df['Time'], df['Conflict Chance'], color='k', alpha=0.6, label="Group Conflict Chance")
#ax.plot(df['Time'], df['Proportion of Cooperators'], color='b', label="Proportion of Cooperators")
ax.plot(df['Time'], moving_average_smoother(df['Proportion of Cooperators'].to_numpy(), 100), 'k', label="Moving Average of Proportion of Cooperators")
ax.plot(df['Time'], moving_average_smoother(df['Average Tax Rate'].to_numpy(), 100), 'g', label="Moving Average of Average Tax Rate")
ax.plot(df['Time'], moving_average_smoother(df['Average Segmentation Rate'].to_numpy(), 100), 'b', label="Moving Average of Average Segmentation Rate")


for i in range(len(df['Shock'].values)):
    if df['Shock'].values[i] == 1:
        ax.axvline(df['Time'].values[i], color='k', alpha=0.3)


ax.set_xlabel("Generation")


#f.suptitle("Coevolution of Institutions and Preferences")
f.legend(loc='lower right')
plt.tight_layout()
plt.show()
