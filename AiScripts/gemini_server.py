from mcp.server.fastmcp import FastMCP
from google import genai
import os

mcp = FastMCP("gemini")

# UPDATED LINE: Using "gemini-1.5-pro-002" which is the specific stable 002 release
@mcp.tool()
def ask_gemini(prompt: str, model: str = "gemini-1.5-pro-002") -> str:
    """
    Ask Gemini to analyze code.
    Args:
        prompt: The text to analyze.
        model: Defaults to "gemini-1.5-pro-002" (Stable Sept 2024 release).
    """
    api_key = os.environ.get("GEMINI_API_KEY")
    if not api_key:
        return "Error: GEMINI_API_KEY not found."

    try:
        # The new Google Gen AI SDK (v1.0+) uses this client structure
        client = genai.Client(api_key=api_key)
        
        response = client.models.generate_content(
            model=model,
            contents=prompt
        )
        return response.text
    except Exception as e:
        return f"Gemini API Error: {str(e)}"

if __name__ == "__main__":
    mcp.run(transport='stdio')
