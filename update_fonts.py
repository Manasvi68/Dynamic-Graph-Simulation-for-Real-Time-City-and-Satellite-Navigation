import re
import codecs

css_path = r'c:\Users\aditk\OneDrive\Desktop\DSA PROJECT\Dynamic-Graph-Simulation-for-Real-Time-City-and-Satellite-Navigation\frontend\src\index.css'
with codecs.open(css_path, 'r', 'utf-8') as f:
    css_content = f.read()

css_content = css_content.replace('https://fonts.googleapis.com/css2?family=Inter:wght@400;500;600;700', 'https://fonts.googleapis.com/css2?family=Plus+Jakarta+Sans:wght@400;500;600;700')
css_content = css_content.replace('family: Inter,', "family: 'Plus Jakarta Sans',")

css_content = re.sub(r'\.input-dark \{.*?font-size: 0\.875rem;', '.input-dark {\n  width: 100%;\n  border-radius: 0.5rem;\n  border: 1px solid rgba(255, 255, 255, 0.16);\n  background: #121722;\n  padding: 0.5rem 0.7rem;\n  font-size: 0.95rem;', css_content, flags=re.DOTALL)

css_content = re.sub(r'\.btn-accent \{.*?font-size: 0\.875rem;', '.btn-accent {\n  border-radius: 0.5rem;\n  background: linear-gradient(180deg, #22c3ee 0%, #0284c7 100%);\n  padding: 0.65rem 0.85rem;\n  font-size: 1rem;', css_content, flags=re.DOTALL)

with codecs.open(css_path, 'w', 'utf-8') as f:
    f.write(css_content)

app_path = r'c:\Users\aditk\OneDrive\Desktop\DSA PROJECT\Dynamic-Graph-Simulation-for-Real-Time-City-and-Satellite-Navigation\frontend\src\App.jsx'
with codecs.open(app_path, 'r', 'utf-8') as f:
    app_content = f.read()

app_content = app_content.replace('text-[10px]', 'text_10px_temp')
app_content = app_content.replace('text-xs', 'text_xs_temp')
app_content = app_content.replace('text-sm', 'text_sm_temp')

app_content = app_content.replace('text_sm_temp', 'text-base')
app_content = app_content.replace('text_xs_temp', 'text-sm')
app_content = app_content.replace('text_10px_temp', 'text-xs')

with codecs.open(app_path, 'w', 'utf-8') as f:
    f.write(app_content)

print('Updated fonts and sizes in CSS and App.jsx')
