#!/usr/bin/env python
# coding: utf-8

# # TFG

# In[15]:


import numpy as np
import pandas as pd
import seaborn as sns
import matplotlib.pyplot as plt
from sklearn import preprocessing


# Importamos dataset

# In[16]:


dataset = pd.read_csv(r"D:\TFG\Bdwifis.csv", encoding='latin1')


# ### Análisis Inicial

# Mostramos las primeras lineas del dataset para ver la estructura y alguna información extra para ver si hay problemas previos

# In[17]:


pd.set_option('display.max_columns', None)
dataset.head(30)


# In[18]:


dataset.describe()


# ### Preprocessing

# Vemos que hay muchas columnas con valores string, asi que hay que transformalos a ints para evitar futuros problemas a la hora de entrenar. Tambien creamos un diccionario con la relacion de las variables para poder identificarlas en un futuro

# In[19]:


variablesStr = ['Aula','SSID','Hora','BSSID']
label_mappings = {}

for variable in variablesStr:
    le = preprocessing.LabelEncoder()
    le.fit(dataset[variable])
    dataset[variable] = le.transform(dataset[variable])
    
    label_mappings[variable] = dict(zip(le.classes_, le.transform(le.classes_)))


# In[20]:


pd.set_option('display.max_columns', None)
dataset.head(30)


# como queremos la información unicamente de la facultad, no tendremos en cuenta las wifis que no sean eduroam ni UAB, buscamos a que valores pertenecen esos casos y no tenemos en cuenta el resto

# In[21]:


print("Valores de 'SSID':")
for original, encoded in label_mappings['SSID'].items():
    print(f"{original} -> {encoded}")


# Vemos que los valores que pertenecen a eduroam y UAB son 21 y 17  respectivamente

# In[25]:


clean_dataset = dataset[(dataset['SSID'] == 21) | (dataset['SSID'] == 17)]


# In[26]:


pd.set_option('display.max_columns', None)
clean_dataset.head(30)


# Ahora que ya tenemos un dataset con la información que queremos vamos a ver las correlaciones para ver que variables nos seran más útiles

# In[27]:


correlacio = clean_dataset.corr()

plt.figure()

ax = sns.heatmap(correlacio, annot=True, linewidths=.5 ,annot_kws={'fontsize':5})


# Podemos ver que tenemos unas correlaciones bastante bajas, ya que la variable objetivo es Aula. Se observa también que la variable no nos importa debido a su tan baja correlación y porque la hora no afecta al aula donde estemos, asi que vamos a eliminarla para que no cause problemas.

# In[28]:


clean_dataset = clean_dataset.drop(['Hora'],axis = 1)


# En este caso, no nos va a hacer falta normalizar las variables, ya que no tienen unos valores muy dispares, a excepción del BSSID, pero esta variable ya esta correcta.

# Finalmente la 

# In[29]:


dataset.describe()


# In[ ]:





# In[ ]:





# In[ ]:





# In[47]:


prueba = clean_dataset


prueba['SSID_BSSID'] = prueba['SSID'].astype(str) + "_" + prueba['BSSID'].astype(str)

# Crear la tabla pivotada
df_pivot = prueba.pivot_table(index='Aula', columns='SSID_BSSID', values='Intensity', aggfunc='max').fillna(-200)

# Mostrar el DataFrame pivotado
print(df_pivot)


# In[ ]:




