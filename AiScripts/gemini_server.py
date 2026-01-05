from mcp.server.fastmcp import FastMCP
import google.generativeai as genai
import os

# Initialize the MCP server
mcp = FastMCP("gemini")

@mcp.tool()
def ask_gemini(prompt: str) -> str:
    """
    Ask Gemini to analyze code or answer questions. 
    Uses the Gemini 1.5 Pro model (Stable).
    
    Args:
        prompt: The text or code to analyze.
    """
    # Check for API Key
    api_key = os.environ.get("GEMINI_API_KEY")
    if not api_key:
        return "Error: GEMINI_API_KEY environment variable not found."

    try:
        # Configure the standard library
        genai.configure(api_key=api_key)
        
        # Initialize the model
        # Note: We use the explicit latest stable version to avoid 404s
        model = genai.GenerativeModel("gemini-2.0-flash")
        
        response = model.generate_content(prompt)
        return response.text

    except Exception as e:
        return f"Gemini API Error: {str(e)}"

if __name__ == "__main__":
    mcp.run(transport='stdio')
